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

hd_info_t hd_drvs[NR_DRIVES];
static void *hd_sata_buf;

static hd_queue_t hdque;
static semaphore_t hdque_mutex;
static semaphore_t hdque_empty;
static semaphore_t hdque_full;

hd_info_t *hd_lookup(int dev) {
    const int type = HD_DEV_GET_DRIVE(dev);
    const int index = HD_DEV_GET_INDEX(dev);
    for (int i = 0; i < NR_DRIVES; ++i) {
        struct hd_info *drv = &hd_drvs[i];
        if (drv->open_cnt == 0) { continue; }
        if (drv->type != type || drv->index != index) { continue; }
        return drv;
    }
    return NULL;
}

hd_part_info_t *hd_lookup_part(int dev) {
    hd_info_t *drv = hd_lookup(dev);
    const int part = HD_DEV_GET_PART(dev);
    assert(part < NR_DRIVE_PARTS);
    //! TODO: support hd without mbr
    hd_part_info_t *info = &drv->parts[part + 1];
    return info->size == 0 ? NULL : info;
}

hd_info_t *hd_get_slot() {
    for (int i = 0; i < NR_DRIVES; ++i) {
        struct hd_info *drv = &hd_drvs[i];
        if (drv->open_cnt > 0) { continue; }
        return drv;
    }
    return NULL;
}

int hd_get_fstype(int dev) {
    const hd_part_info_t *part = hd_lookup_part(dev);
    return part ? part->fs_type : -1;
}

/*!
 * \brief detect the fs type for a hd block start at the given sectors
 */
static int hd_blk_detect_fstype(int dev, size_t nr_sector) {
    for (int type = 0; type < NR_FS_TYPE; ++type) {
        if (!fstype_table[type].identify) { continue; }
        const bool matched = fstype_table[type].identify(dev, nr_sector);
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

void init_hd() {
    memset(hd_drvs, 0, sizeof(hd_drvs));

    hd_queue_init(&hdque);
    ksem_init(&hdque_mutex, 1);
    ksem_init(&hdque_empty, HDQUE_MAXSIZE);
    ksem_init(&hdque_full, 0);

    hd_sata_buf = kern_kmalloc(BLOCK_SIZE);
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
    for (size_t i = 0; i < ahci_active()->total_sata_drv; ++i) {
        hd_open(HD_DEV_MAKE_BLK(DEV_BLK_HD_SATA, i));
    }
}

static hd_messge_t *hd_build_drv_msg(int io_type, int dev, u64 pos, int bytes, int proc_nr,
                                     void *buf) {
    hd_messge_t *msg = kern_kmalloc(sizeof(hd_messge_t));
    assert(msg != NULL);
    msg->type = io_type;
    msg->dev = dev;
    msg->pos = pos;
    msg->cnt = bytes;
    msg->pid = proc_nr;
    msg->buf = buf;
    return msg;
}

static int sata_rdwt_impl(int dev, int type, u64 nr_sector, u32 count) {
    assert(HD_DEV_IS_BLK(dev));
    assert(HD_DEV_GET_DRIVE(dev) == DEV_BLK_HD_SATA);
    const size_t nr_sata_drv = HD_DEV_GET_INDEX(dev);
    assert(nr_sata_drv < ahci_active()->total_sata_drv);
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
    cmd_hdr->w = type == HD_CMD_READ ? 0 : 1;
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

    cmdfis->command = type == HD_CMD_READ ? ATA_CMD_READ_DMA_EX : ATA_CMD_WRITE_DMA_EX;

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

    if (unlikely(kstate_on_init)) {
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

static void hd_rdwt_impl_sata(int dev, int type, u64 nr_sector, u32 count, void *buf) {
    const size_t nr_sectors = ROUNDUP(count, SECTOR_SIZE) / SECTOR_SIZE;
    if (type == HD_CMD_WRITE) { memcpy(hd_sata_buf, buf, count); }
    const int retval = sata_rdwt_impl(dev, type, nr_sector, nr_sectors);
    assert(retval == 0);
    if (type == HD_CMD_READ) { memcpy(buf, hd_sata_buf, count); }
}

static void hd_rdwt_router(int dev, int type, size_t nr_sector, size_t count, void *buf) {
    assert(HD_DEV_IS_BLK(dev));
    switch (HD_DEV_GET_DRIVE(dev)) {
        case DEV_BLK_HD_SATA: {
            hd_rdwt_impl_sata(dev, type, nr_sector, count, buf);
        } break;
        default: {
            unreachable();
        } break;
    }
}

void hd_rdwt(hd_messge_t *p) {
    const hd_part_info_t *part = hd_lookup_part(p->dev);
    assert(part != NULL);
    size_t nr_sector = p->pos / SECTOR_SIZE;
    assert(nr_sector < part->size);
    nr_sector += part->base;
    void *la = va2la(p->pid, p->buf);
    hd_rdwt_router(HD_DEV_TO_BLK(p->dev), p->type, nr_sector, p->cnt, la);
}

void hd_rdwt_sched(hd_messge_t *p) {
    hd_rwinfo_t *rwinfo = kern_kmalloc(sizeof(hd_rwinfo_t));
    rwinfo->msg = p;
    rwinfo->proc = p_proc_current;
    assert(p->type == HD_CMD_READ || p->type == HD_CMD_WRITE);
    hd_queue_enqueue(&hdque, rwinfo);
    wait_event(&hdque);
}

static void hd_load_mbr_partition(int dev) {
    assert(HD_DEV_IS_BLK(dev));
    hd_info_t *drv = hd_lookup(dev);
    assert(drv != NULL);

    void *sector_buf = kern_kmalloc(SECTOR_SIZE);
    assert(sector_buf != NULL);

    const struct mbr_part_ent *part_tbl = sector_buf + MBR_PART_TABLE_OFFSET;
    hd_rdwt_router(dev, HD_CMD_READ, 0, SECTOR_SIZE, sector_buf);

    int nr_prim_parts = 0;
    int nr_part_extended = -1;

    for (int i = 0; i < NR_MBR_PRIM_PART; ++i) {
        if (part_tbl[i].sys_id == MBR_PART_NONE) { continue; }
        if (part_tbl[i].sys_id == MBR_PART_EXT) {
            assert(nr_part_extended == -1);
            nr_part_extended = i;
        }
        const int nr_dev = i + 1;
        drv->parts[nr_dev].base = part_tbl[i].start_sect;
        drv->parts[nr_dev].size = part_tbl[i].nr_sects;
        drv->parts[nr_dev].fs_type = hd_blk_detect_fstype(dev, drv->parts[nr_dev].base);
        ++nr_prim_parts;
        ++drv->nr_parts;
    }

    if (nr_part_extended != -1) {
        const int nr_dev = nr_part_extended + 1;
        const int nr_ext_part_base = 5;
        const int nr_ext_sector = drv->parts[nr_dev].base;
        int logical_part_off = 0;
        for (int i = 0; i < NR_MBR_EXT_PART; ++i) {
            const int nr_subdev = nr_ext_part_base + i;
            const int nr_sector = nr_ext_sector + logical_part_off;
            hd_rdwt_router(dev, HD_CMD_READ, nr_sector, SECTOR_SIZE, sector_buf);
            drv->parts[nr_subdev].base = nr_sector + part_tbl[0].start_sect;
            drv->parts[nr_subdev].size = part_tbl[0].nr_sects;
            ++drv->nr_parts;
            if (part_tbl[1].sys_id == MBR_PART_NONE) { break; }
            logical_part_off = part_tbl[1].start_sect;
        }
    }

    //! TODO: support non-mbr hd
    assert(nr_prim_parts > 0);

    kern_kfree(sector_buf);
}

void hd_open(int dev) {
    assert(HD_DEV_GET_DRIVE(dev) == DEV_BLK_HD_SATA);
    assert(HD_DEV_GET_INDEX(dev) < ahci_active()->total_sata_drv);
    hd_info_t *drv = hd_lookup(dev);
    if (drv != NULL) {
        atomic_inc(&drv->open_cnt);
        return;
    }
    drv = hd_get_slot();
    assert(drv != NULL);
    atomic_inc(&drv->open_cnt);
    drv->type = HD_DEV_GET_DRIVE(dev);
    drv->index = HD_DEV_GET_INDEX(dev);
    const int nr_port = ahci_active()->sata_drv_port[drv->index];
    void *buf = kern_kmalloc(SECTOR_SIZE);
    assert(buf != NULL);
    identity_sata(&ahci_hba()->ports[nr_port], buf);
    u16 *hdinfo = buf;
    drv->parts[0].base = 0;
    drv->parts[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
    kern_kfree(buf);
    hd_load_mbr_partition(dev);
}

void hd_close(int dev) {
    hd_info_t *drv = hd_lookup(dev);
    assert(drv != NULL);
    atomic_dec(&drv->open_cnt);
    if (drv->open_cnt == 0) { memset(drv, 0, sizeof(hd_info_t)); }
}

int hd_make_part_dev(int dev, int part, int fs_type) {
    assert(hd_lookup(dev) != NULL);
    assert(hd_lookup(dev)->parts[part + 1].size > 0);
    assert(hd_lookup(dev)->parts[part + 1].fs_type == fs_type);
    return HD_DEV_MAKE(HD_DEV_GET_DRIVE(dev), HD_DEV_GET_INDEX(dev), part);
}

int hd_find_dev_of_fstype(int dev, int fs_type) {
    hd_info_t *drv = hd_lookup(dev);
    assert(drv != NULL);
    for (int i = 0, j = 0; i < NR_DRIVE_PARTS && j < (ssize_t)drv->nr_parts; ++i) {
        if (drv->parts[i + 1].size > 0) { ++j; }
        if (drv->parts[i + 1].fs_type != fs_type) { continue; }
        return hd_make_part_dev(dev, i, fs_type);
    }
    return -1;
}

void rw_blocks(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf) {
    hd_rdwt(hd_build_drv_msg(io_type, dev, pos, bytes, proc_nr, buf));
}

void rw_blocks_sched(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf) {
    hd_rdwt_sched(hd_build_drv_msg(io_type, dev, pos, bytes, proc_nr, buf));
}

int orangefs_identify(int dev, u32 nr_sect) {
    struct {
        u8 orange_flag;
        u16 reserved;
        u32 fat32_flag1;
        u32 fat32_flag2;
    } fs_flags_real;
    hd_rdwt_router(dev, HD_CMD_READ, nr_sect + 8, sizeof(fs_flags_real), &fs_flags_real);
    return fs_flags_real.orange_flag == 0x11;
}

int fat32_identify(int dev, u32 nr_sect) {
    char sect_buf[SECTOR_SIZE] = {};
    hd_rdwt_router(dev, HD_CMD_READ, nr_sect, SECTOR_SIZE, sect_buf);
    const char *fat32_flag = (void *)(sect_buf + 0x52);
    return strncmp(fat32_flag, "FAT32   ", 8) == 0;
}

int ext4_identify(int dev, u32 nr_sect) {
    char sect_buf[SECTOR_SIZE] = {};
    // 这里要 +2 因为前 1024 字节是填充 0
    hd_rdwt_router(dev, HD_CMD_READ, nr_sect + 2, SECTOR_SIZE, sect_buf);
    const u16 magic = *(u16 *)(sect_buf + 0x38);
    return magic == 0xef53;
}
