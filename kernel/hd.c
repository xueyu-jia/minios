#include <minios/hd.h>
#include <minios/dev.h>
#include <minios/memman.h>
#include <minios/console.h>
#include <minios/proc.h>
#include <minios/sched.h>
#include <minios/interrupt.h>
#include <minios/semaphore.h>
#include <minios/clock.h>
#include <minios/kstate.h>
#include <minios/assert.h>
#include <minios/asm.h>
#include <minios/vfs.h>
#include <minios/layout.h>
#include <driver/ata/ahci.h>
#include <fs/fs.h>
#include <string.h>

struct hd_info hd_infos[12] = {};
static bool hd_lba48_sup[4] = {};

static volatile int hd_int_waiting_flag = 1;
static volatile uint8_t hd_status = 0;

static u8 hdbuf[SECTOR_SIZE * 2] = {};
static void *hd_sata_buf = NULL;

static hd_queue_t hdque = {};
static semaphore_t hdque_mutex = {};
static semaphore_t hdque_empty = {};
static semaphore_t hdque_full = {};

#define DRV_OF_DEV(dev) (MAJOR(dev) - DEV_HD_BASE)

int hd_get_fstype(int dev) {
    return hd_infos[MAJOR(dev) - DEV_HD_BASE].part[MINOR(dev)].fs_type;
}

/*!
 * \brief detect the fs type for a hd block start at the given sectors
 */
static int hd_blk_detect_fstype(int drive, size_t nr_sector) {
    for (int type = 0; type < NR_FS_TYPE; ++type) {
        if (!fstype_table[type].identify) { continue; }
        const bool matched = fstype_table[type].identify(drive, nr_sector);
        if (matched) { return type; }
    }
    return FS_TYPE_NONE;
}

static void hd_queue_init(hd_queue_t *q) {
    q->front = NULL;
    q->rear = NULL;
}

static void hd_queue_enqueue(hd_queue_t *q, hd_rwinfo_t *rwinfo) {
    ksem_wait(&hdque_empty, 1);
    ksem_wait(&hdque_mutex, 1);

    rwinfo->next = NULL;
    if (q->rear == NULL) {
        q->front = rwinfo;
        q->rear = rwinfo;
    } else {
        q->rear->next = rwinfo;
        q->rear = rwinfo;
    }

    ksem_post(&hdque_mutex, 1);
    ksem_post(&hdque_full, 1);
}

static void hd_queue_dequeue(hd_queue_t *q, hd_rwinfo_t **rwinfo) {
    ksem_wait(&hdque_full, 1);
    ksem_wait(&hdque_mutex, 1);

    *rwinfo = q->front;
    if (q->front == q->rear) {
        q->front = NULL;
        q->rear = NULL;
    } else {
        q->front = q->front->next;
    }

    ksem_post(&hdque_mutex, 1);
    ksem_post(&hdque_empty, 1);
}

static void hd_wait_int() {
    while (hd_int_waiting_flag) {}
    hd_int_waiting_flag = 1;
}

static void hd_notify_int() {
    hd_int_waiting_flag = 0;
}

static void hd_handler(int irq) {
    UNUSED(irq);
    // Interrupts are cleared when the host
    // - reads the Status Register,
    // - issues a reset, or
    // - writes to the Command Register.
    hd_status = inb(REG_STATUS);
    hd_notify_int();
}

void init_hd() {
    memset(hd_infos, 0, sizeof(hd_infos));

    hd_queue_init(&hdque);
    ksem_init(&hdque_mutex, 1);
    ksem_init(&hdque_empty, HDQUE_MAXSIZE);
    ksem_init(&hdque_full, 0);

    hd_sata_buf = kern_kmalloc(BLOCK_SIZE);

    put_irq_handler(AT_WINI_IRQ, hd_handler);
    enable_irq(CASCADE_IRQ);
    enable_irq(AT_WINI_IRQ);
}

void hd_service() {
    while (true) {
        hd_rwinfo_t *rwinfo = NULL;
        hd_queue_dequeue(&hdque, &rwinfo);
        assert(rwinfo != NULL);
        hd_rdwt(rwinfo->msg);
        kern_kfree(rwinfo->msg);
        kern_kfree(rwinfo);
        wakeup(&hdque);
    }
}

void hd_open_and_init() {
    for (size_t i = 0; i < ahci_active()->total_sata_drv; ++i) { hd_open(SATA_BASE + i); }
}

/*!
 * \brief DEV_IOCTL msg handler
 */
void hd_ioctl(hd_messge_t *p) {
    const int drive = DRV_OF_DEV(p->DEVICE);
    const int nr_part = p->DEVICE & 0x0fffff;

    struct hd_info *info = &hd_infos[drive];

    switch (p->REQUEST) {
        case DIOCTL_GET_GEO: {
            void *dst = va2la(p->PROC_NR, p->BUF);
            void *src = va2la(proc2pid(p_proc_current), &info->part[nr_part]);
            memcpy(dst, src, sizeof(struct part_info));
        } break;
        default: {
            unreachable();
        } break;
    }
}

static hd_messge_t *hd_build_drv_msg(int io_type, int dev, u64 pos, int bytes, int proc_nr,
                                     void *buf) {
    hd_messge_t *msg = kern_kmalloc(sizeof(hd_messge_t));
    assert(msg != NULL);

    msg->type = io_type;
    msg->DEVICE = dev;
    msg->POSITION = pos;
    msg->CNT = bytes;
    msg->PROC_NR = proc_nr;
    msg->BUF = buf;

    return msg;
}

static bool hd_wait_status(int mask, int expected, int timeout) {
    const int beg = kern_clock();
    while (true) {
        if ((hd_status & mask) == expected) { return true; }
        const int end = kern_clock();
        if ((end - beg) * 1000 / CLOCKS_PER_SEC >= timeout) { break; }
    }
    return false;
}

static void hd_post_cmd(hd_cmd_t *cmd, int drive) {
    /**
     * For all commands, the host must first check if BSY=1,
     * and should proceed no further unless and until BSY=0
     */
    while (true) {
        const bool busy = !hd_wait_status(STATUS_BSY, 0, HD_TIMEOUT);
        if (!busy) { break; }
    }

    /* Activate the Interrupt Enable (nIEN) bit */
    outb(REG_DEV_CTRL, 0);

    // 若是大硬盘，则LBA48第一次写
    if (hd_lba48_sup[drive]) {
        outb(REG_FEATURES, cmd->features);
        outb(REG_NSECTOR, cmd->count_LBA48);
        outb(REG_LBA_LOW, cmd->lba_low_LBA48);
        outb(REG_LBA_MID, cmd->lba_mid_LBA48);
        outb(REG_LBA_HIGH, cmd->lba_high_LBA48);
    }

    /* Load required parameters in the Command Block Registers */
    outb(REG_FEATURES, cmd->features);
    outb(REG_NSECTOR, cmd->count);
    outb(REG_LBA_LOW, cmd->lba_low);
    outb(REG_LBA_MID, cmd->lba_mid);
    outb(REG_LBA_HIGH, cmd->lba_high);
    outb(REG_DEVICE, cmd->device);
    /* Write the command code to the Command Register */
    outb(REG_CMD, cmd->command);
}

static void hd_rdwt_impl_ide(int drive, int type, u64 nr_sector, u32 count, void *buf) {
    const size_t nr_sectors = ROUNDUP(count, SECTOR_SIZE) / SECTOR_SIZE;

    hd_cmd_t cmd = {};
    cmd.features = 0;
    cmd.count = nr_sectors & 0xff;
    cmd.lba_low = nr_sector & 0xff;
    cmd.lba_mid = (nr_sector >> 8) & 0xff;
    cmd.lba_high = (nr_sector >> 16) & 0xff;
    if (hd_lba48_sup[drive]) { // LBA48
        assert(nr_sectors < BIT(16));
        cmd.count_LBA48 = (nr_sectors >> 8) & 0xff;
        cmd.lba_low_LBA48 = (nr_sector >> 24) & 0xff;
        cmd.lba_mid_LBA48 = (nr_sector >> 32) & 0xff;
        cmd.lba_high_LBA48 = (nr_sector >> 40) & 0xff;
        // 0~3位,0；第4位0表示主盘,1表示从盘；7~5位,010,表示为LBA
        cmd.device = 0x40 | ((drive << 4) & 0xff);
        cmd.command = (type == DEV_READ) ? ATA_READ_EXT : ATA_WRITE_EXT;
    } else { // LBA28
        assert(nr_sectors < BIT(8));
        cmd.device = MAKE_DEVICE_REG(1, drive, (nr_sector >> 24) & 0xf);
        cmd.command = (type == DEV_READ) ? ATA_READ : ATA_WRITE;
    }
    hd_post_cmd(&cmd, drive);

    ssize_t bytes_left = count;
    void *ptr = buf;
    if (type == DEV_READ) {
        while (bytes_left > 0) {
            const size_t bytes = MIN(SECTOR_SIZE, bytes_left);
            hd_wait_int();
            insw(REG_DATA, hdbuf, SECTOR_SIZE);
            memcpy(ptr, hdbuf, bytes);
            bytes_left -= bytes;
            ptr += bytes;
        }
    } else {
        while (bytes_left > 0) {
            const size_t bytes = MIN(SECTOR_SIZE, bytes_left);
            const bool ready = hd_wait_status(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT);
            assert(ready);
            memcpy(hdbuf, ptr, bytes);
            outsw(REG_DATA, hdbuf, SECTOR_SIZE);
            hd_wait_int();
            bytes_left -= bytes;
            ptr += bytes;
        }
    }
}

static int sata_rdwt_impl(int drive, int type, u64 nr_sector, u32 count) {
    assert(drive >= SATA_BASE && drive < SATA_LIMIT);
    const int nr_sata_drv = drive - SATA_BASE;
    int nr_port = ahci_active()->sata_drv_port[nr_sata_drv];
    HBA_PORT *port = &ahci_hba()->ports[nr_port];

    // Clear pending interrupt bits
    port->is = 0xffffffff;

    int slot = ahci_find_cmd_slot(port);
    if (slot == -1) { return -1; }

    HBA_CMD_HEADER *cmd_hdr = K_PHY2LIN(port->clb);
    cmd_hdr += slot;
    // Command FIS size
    cmd_hdr->cfl = sizeof(FIS_REG_H2D) / sizeof(u32);
    // 0:device to host,read ;1:host to device,write
    cmd_hdr->w = (type == DEV_READ) ? 0 : 1;
    cmd_hdr->c = 0;
    // Software shall not set CH(pFreeSlot).P when building queued ATA commands.
    cmd_hdr->p = 0;
    cmd_hdr->prdbc = 0;
    // PRDT entries count
    cmd_hdr->prdtl = 1;

    HBA_CMD_TBL *cmd_tbl = K_PHY2LIN(cmd_hdr->ctba);
    memset(cmd_tbl, 0, sizeof(HBA_CMD_TBL) + (cmd_hdr->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

    int i = 0;
    for (i = 0; i < cmd_hdr->prdtl - 1; ++i) {
        cmd_tbl->prdt_entry[i].dba = K_LIN2PHY(hd_sata_buf);
        // 8K bytes (this value should always be set to 1 less than the actual value)
        cmd_tbl->prdt_entry[i].dbc = SZ_8K - 1;
        cmd_tbl->prdt_entry[i].i = 0;
        hd_sata_buf += SZ_8K;
        count -= 16;
    }
    cmd_tbl->prdt_entry[i].dba = K_LIN2PHY(hd_sata_buf);
    cmd_tbl->prdt_entry[i].dbc = count * SECTOR_SIZE - 1;
    cmd_tbl->prdt_entry[i].i = 0;

    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D *)(&cmd_tbl->cfis);

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1; // Command

    cmdfis->command = type == DEV_READ ? ATA_CMD_READ_DMA_EX : ATA_CMD_WRITE_DMA_EX;

    cmdfis->lba0 = nr_sector & 0xff;
    cmdfis->lba1 = (nr_sector >> 8) & 0xff;
    cmdfis->lba2 = (nr_sector >> 16) & 0xff;
    cmdfis->device = 1 << 6; // LBA mode

    cmdfis->lba3 = (nr_sector >> 24) & 0xff;
    cmdfis->lba4 = (nr_sector >> 32) & 0xff;
    cmdfis->lba5 = (nr_sector >> 40) & 0xff;

    cmdfis->countl = count & 0xff;
    cmdfis->counth = (count >> 8) & 0xff;

    // The below loop waits until the port is no longer busy before issuing a new command
    const int max_spin = 1000000;
    int spin = 0; // Spin lock timeout counter
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < max_spin) { ++spin; }
    if (spin == max_spin) { return -1; }

    if (kstate_on_init) {
        port->ci = 1 << slot; // Issue command
        while (sata_wait_flag) {}
        sata_wait_flag = 1;
    } else {
        /*此处采用开关中断的设计是为了防止sata中断在将hd_service设置为SLEEPING前到来*/
        disable_int_begin();
        port->ci = 1 << slot; // Issue command
        wait_event((void *)&sata_wait_flag);
        disable_int_end();
    }

    return sata_error_flag == 1 ? -1 : 0;
}

static void hd_rdwt_impl_sata(int drive, int type, u64 nr_sector, u32 count, void *buf) {
    const size_t nr_sectors = ROUNDUP(count, SECTOR_SIZE) / SECTOR_SIZE;
    if (type == DEV_WRITE) { memcpy(hd_sata_buf, buf, count); }
    const int retval = sata_rdwt_impl(drive, type, nr_sector, nr_sectors);
    assert(retval == 0);
    if (type == DEV_READ) { memcpy(buf, hd_sata_buf, count); }
}

static void hd_rdwt_router(int drive, int type, size_t nr_sector, size_t count, void *buf) {
    if (drive >= SATA_BASE && drive < SATA_LIMIT) {
        hd_rdwt_impl_sata(drive, type, nr_sector, count, buf);
    } else if (drive >= IDE_BASE && drive < IDE_LIMIT) {
        hd_rdwt_impl_ide(drive, type, nr_sector, count, buf);
    } else {
        unreachable();
    }
}

void hd_rdwt(hd_messge_t *p) {
    int drive = DRV_OF_DEV(p->DEVICE);
    u64 pos = p->POSITION;
    struct part_info *info = &hd_infos[drive].part[p->DEVICE & 0x0f];
    u32 nr_sector = (pos >> SECTOR_SIZE_SHIFT);
    assert(nr_sector < info->size);
    nr_sector += info->base;
    void *la = (void *)va2la(p->PROC_NR, p->BUF);
    hd_rdwt_router(drive, p->type, nr_sector, p->CNT, la);
}

void hd_rdwt_sched(hd_messge_t *p) {
    hd_rwinfo_t *rwinfo = kern_kmalloc(sizeof(hd_rwinfo_t));
    int size = p->CNT;
    UNUSED(size);

    rwinfo->msg = p;
    rwinfo->proc = p_proc_current;

    assert(p->type == DEV_READ || p->type == DEV_WRITE);
    hd_queue_enqueue(&hdque, rwinfo);
    wait_event(&hdque);
}

static void hd_load_mbr_partition(int device) {
    const int drive = DRV_OF_DEV(device);
    struct hd_info *info = &hd_infos[drive];

    void *sector_buf = kern_kmalloc(SECTOR_SIZE);
    assert(sector_buf != NULL);

    const struct part_ent *part_tbl = sector_buf + PARTITION_TABLE_OFFSET;
    hd_rdwt_router(drive, DEV_READ, 0, SECTOR_SIZE, sector_buf);

    int nr_prim_parts = 0;
    int nr_part_extended = -1;

    for (int i = 0; i < NR_PART_PER_DRIVE; ++i) {
        if (part_tbl[i].sys_id == NO_PART) { continue; }
        if (part_tbl[i].sys_id == EXT_PART) {
            assert(nr_part_extended == -1);
            nr_part_extended = i;
        }
        const int nr_dev = i + 1;
        info->part[nr_dev].base = part_tbl[i].start_sect;
        info->part[nr_dev].size = part_tbl[i].nr_sects;
        info->part[nr_dev].fs_type = hd_blk_detect_fstype(drive, info->part[nr_dev].base);
        ++nr_prim_parts;
    }

    if (nr_part_extended != -1) {
        const int nr_dev = nr_part_extended + 1;
        const int nr_ext_part_base = 5;
        const int nr_ext_sector = info->part[nr_dev].base;
        int logical_part_off = 0;
        for (int i = 0; i < NR_SUB_PER_PART; ++i) {
            const int nr_subdev = nr_ext_part_base + i;
            const int nr_sector = nr_ext_sector + logical_part_off;
            hd_rdwt_router(drive, DEV_READ, nr_sector, SECTOR_SIZE, sector_buf);
            info->part[nr_subdev].base = nr_sector + part_tbl[0].start_sect;
            info->part[nr_subdev].size = part_tbl[0].nr_sects;
            if (part_tbl[1].sys_id == NO_PART) { break; }
            logical_part_off = part_tbl[1].start_sect;
        }
    }

    //! TODO: support non-mbr hd
    assert(nr_prim_parts > 0);

    kern_kfree(sector_buf);
}

static void hd_identify(int drive) {
    hd_cmd_t cmd = {
        .device = MAKE_DEVICE_REG(0, drive, 0),
        .command = ATA_IDENTIFY,
    };
    hd_post_cmd(&cmd, drive);
    hd_wait_int();
    insw(REG_DATA, hdbuf, SECTOR_SIZE);

    u16 *hdinfo = (u16 *)hdbuf;

    const int cmd_set_supported = hdinfo[83];
    if (cmd_set_supported & 0x0400) { hd_lba48_sup[drive] = true; }

    hd_infos[drive].part[0].base = 0;
    hd_infos[drive].part[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
}

void hd_open(int drive) {
    if (hd_infos[drive].open_cnt == 0) {
        if (drive >= SATA_BASE && drive < SATA_LIMIT) {
            const int nr_port = ahci_active()->sata_drv_port[drive - SATA_BASE];
            void *buf = kern_kmalloc(SECTOR_SIZE);
            assert(buf != NULL);
            identity_sata(&ahci_hba()->ports[nr_port], buf);
            u16 *hdinfo = buf;
            hd_infos[drive].part[0].base = 0;
            hd_infos[drive].part[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
            kern_kfree(buf);
        } else if (drive < SATA_BASE) {
            hd_identify(drive);
        }
        hd_load_mbr_partition(MAKE_DEV(drive + DEV_HD_BASE, 0));
    }
    atomic_inc(&hd_infos[drive].open_cnt);
}

void hd_close(int drive) {
    atomic_dec(&hd_infos[drive].open_cnt);
}

int hd_find_dev_of_fstype(int drive, int fs_type) {
    for (int i = 1; i < NR_PRIM_PER_DRIVE; ++i) {
        if (hd_infos[drive].part[i].fs_type != fs_type) { continue; }
        return MAKE_DEV(DEV_HD_BASE + drive, i);
    }
    for (int i = NR_PRIM_PER_DRIVE; i < NR_PRIM_PER_DRIVE + NR_SUB_PER_PART; ++i) {
        if (hd_infos[drive].part[i].fs_type != fs_type) { continue; }
        return MAKE_DEV(DEV_HD_BASE + drive, i);
    }
    return -1;
}

int hd_make_part_dev(int drive, int part, int fs_type) {
    assert(hd_infos[drive].part[part].fs_type == fs_type);
    return MAKE_DEV(DEV_HD_BASE + drive, part);
}

void rw_blocks(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf) {
    hd_rdwt(hd_build_drv_msg(io_type, dev, pos, bytes, proc_nr, buf));
}

void rw_blocks_sched(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf) {
    hd_rdwt_sched(hd_build_drv_msg(io_type, dev, pos, bytes, proc_nr, buf));
}

int orangefs_identify(int drive, u32 nr_sect) {
    struct fs_flags fs_flags_real;
    hd_rdwt_router(drive, DEV_READ, nr_sect + 8, sizeof(struct fs_flags), &fs_flags_real);
    return fs_flags_real.orange_flag == 0x11;
}

int fat32_identify(int drive, u32 nr_sect) {
    char sect_buf[SECTOR_SIZE] = {};
    hd_rdwt_router(drive, DEV_READ, nr_sect, SECTOR_SIZE, sect_buf);
    const char *fat32_flag = (void *)(sect_buf + 0x52);
    return strncmp(fat32_flag, "FAT32   ", 8) == 0;
}

int ext4_identify(int drive, u32 nr_sect) {
    char sect_buf[SECTOR_SIZE] = {};
    // 这里要 +2 因为前 1024 字节是填充 0
    hd_rdwt_router(drive, DEV_READ, nr_sect + 2, SECTOR_SIZE, sect_buf);
    const u16 magic = *(u16 *)(sect_buf + 0x38);
    return magic == 0xef53;
}
