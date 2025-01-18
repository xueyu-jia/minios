#include <minios/hd.h>
#include <minios/ahci.h>
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
#include <fs/fs.h>
#include <string.h>

static hd_queue_t hdque;
static volatile int hd_int_waiting_flag = 1;
static u8 hd_status;
static u8 hdbuf[SECTOR_SIZE * 2];
static u8 *satabuf = NULL;
static u8 hd_LBA48_sup[4] = {0};
struct hd_info hd_infos[12]; // modified by xiaofeng 2022-8-6
static semaphore_t hdque_mutex;
static semaphore_t hdque_empty;
static semaphore_t hdque_full;

static void init_hd_queue(hd_queue_t *hdq);
static void in_hd_queue(hd_queue_t *hdq, hd_rwinfo_t *p);
static void out_hd_queue(hd_queue_t *hdq, hd_rwinfo_t **p);
static void hd_rdwt_real(hd_rwinfo_t *p);

static void read_part_table_sector(int drive, int sect_nr, void *sector_buf);
static void partition(int device, int style);
static void hd_identify(int drive);
static void print_identify_info(u16 *hdinfo);
static void hd_cmd_out(hd_cmd_t *cmd, int drive);

static void inform_int();
static void interrupt_wait();
static void hd_handler(int irq);
static int waitfor(int mask, int val, int timeout);

static int SATA_rdwt(int drive, int type, u64 sect_nr, u32 count, void *buf);
static void IDE_rdwt(int drive, int type, u64 sect_nr, u32 count, void *buf);
static int SATA_rdwt_sects(int drive, int type, u64 sect_nr, u32 count);

#define DRV_OF_DEV(dev) (((dev >> 20) & 0x0FFF) - DEV_HD_BASE)

static inline void port_read(int port, void *buf, int n) {
    insw(port, buf, n);
}

static inline void port_write(int port, const void *buf, int n) {
    outsw(port, buf, n);
}

/*****************************************************************************
 *                                init_hd
 *****************************************************************************/
/**
 * <Ring 1> Check hard drive, set IRQ handler, enable IRQ and initialize data
 *          structures.
 *****************************************************************************/
void init_hd() {
    put_irq_handler(AT_WINI_IRQ, hd_handler);
    enable_irq(CASCADE_IRQ);
    enable_irq(AT_WINI_IRQ);

    for (size_t i = 0; i < (sizeof(hd_infos) / sizeof(hd_infos[0])); i++)
        memset(&hd_infos[i], 0, sizeof(hd_infos[0]));
    hd_infos[0].open_cnt = 0;

    // init hd rdwt queue. added by xw, 18/8/27
    init_hd_queue(&hdque);
    ksem_init(&hdque_mutex, 1);
    ksem_init(&hdque_empty, HDQUE_MAXSIZE);
    ksem_init(&hdque_full, 0);
}

/*****************************************************************************
 *                                hd_open
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_OPEN message. It identify the drive
 * of the given device and read the partition table of the drive if it
 * has not been read.
 *
 * @param device The device to be opened.
 *****************************************************************************/
// PUBLIC void hd_open(int device) //no need for int device, mingxuan
void hd_open(int drive) // modified by mingxuan 2020-10-27
{
    // kprintf("Read hd information...  ");	//deleted by mingxuan 2021-2-7
    if (satabuf == NULL) satabuf = (u8 *)kern_kmalloc(BLOCK_SIZE);
    /* Get the number of drives from the BIOS data area */
    u8 *pNrDrives = (u8 *)(0x475);
    UNUSED(pNrDrives);

    if (drive >= SATA_BASE && drive < SATA_LIMIT) // SATA
    {
        u8 *buf = (u8 *)kern_kmalloc(512);
        u32 port_num = ahci_info[0].satadrv_atport[drive - SATA_BASE];
        identity_SATA(&(HBA->ports[port_num]), buf);

        // print_identify_info((u16*)buf);
        u16 *hdinfo = (u16 *)buf;
        int cmd_set_supported = hdinfo[83];
        UNUSED(cmd_set_supported);
        hd_infos[drive].part[0].base = 0;
        /* Total Nr of User Addressable Sectors */
        hd_infos[drive].part[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
        kern_kfree((u32)buf);
    } else if (drive < SATA_BASE) {
        hd_identify(drive);
    }
    if (hd_infos[drive].open_cnt++ == 0) { partition(MAKE_DEV(drive + DEV_HD_BASE, 0), P_PRIMARY); }
}

/*****************************************************************************
 *                                hd_close
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_CLOSE message.
 *
 * @param device The device to be opened.
 *****************************************************************************/
void hd_close(int device) {
    int drive = DRV_OF_DEV(device);

    hd_infos[drive].open_cnt--;
}

void init_open_hd() {
    for (u32 dev_index = 0; dev_index < ahci_info[0].satadrv_num; ++dev_index) {
        hd_open(SATA_BASE + dev_index);
    }
}

int get_hd_dev(int drive, u32 fs_type) {
    int i;
    for (i = 1; i < NR_PRIM_PER_DRIVE;
         i++) // 跳过第1个主分区，因为第1个分区是启动分区 comment added by ran
    {
        if (hd_infos[drive].part[i].fs_type == fs_type) return MAKE_DEV(DEV_HD_BASE + drive, i);
    }

    // added by mingxuan 2020-10-29
    for (i = NR_PRIM_PER_DRIVE; i < NR_PRIM_PER_DRIVE + NR_SUB_PER_PART; ++i) {
        if (hd_infos[drive].part[i].fs_type == fs_type) return MAKE_DEV(DEV_HD_BASE + drive, i);
    }
    assert(0);
    return -1;
}

int get_hd_part_dev(int drive, int part, u32 fs_type) {
    if (hd_infos[drive].part[part].fs_type == fs_type) {
        return MAKE_DEV(DEV_HD_BASE + drive, part);
    }
    kprintf("fatal: FSTYPE provided incorrect");
    return -1;
}

u32 get_hd_fstype(int dev) {
    return hd_infos[MAJOR(dev) - DEV_HD_BASE].part[MINOR(dev)].fs_type;
}

// hd_rdwt_base
// 		low level hd read write count bytes, must start from sector
// boundary 		count <= BLOCK_SIZE
int hd_rdwt_base(int drive, int type, u32 sect_nr, u32 count, void *buf) {
    u64 sect = (u64)sect_nr;
    if (drive >= SATA_BASE && drive < SATA_LIMIT) {
        SATA_rdwt(drive, type, sect, count, buf);
        return 1;
    } else if (drive >= IDE_BASE && drive < IDE_LIMIT) {
        IDE_rdwt(drive, type, sect, count, buf);
        return 1;
    }
    return 0;
}

/*****************************************************************************
 *                                hd_rdwt
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_READ and DEV_WRITE message.
 *
 * @param p Message ptr.
 *****************************************************************************/
void hd_rdwt(hd_messge_t *p) {
    int drive = DRV_OF_DEV(p->DEVICE);
    u64 pos = p->POSITION;
    struct part_info *info = &hd_infos[drive].part[p->DEVICE & 0x0F];
    // We only allow to R/W from a SECTOR boundary:
    u32 sect_nr = (pos >> SECTOR_SIZE_SHIFT);
    if (sect_nr > info->size) {
        kprintf("read/write sector out of range!\n");
        return;
    }
    sect_nr += info->base;
    // u32	count =(p->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;

    void *la = (void *)va2la(p->PROC_NR, p->BUF);
    hd_rdwt_base(drive, p->type, sect_nr, p->CNT, la);
}

// added by xw, 18/8/26
void hd_service() {
    hd_rwinfo_t *rwinfo;
    process_t *proc;
    int wait;
    UNUSED(wait);
    UNUSED(proc);
    while (1) {
        // kprintf("-hd-"); //mark debug
        // //the hd queue is not empty when out_hd_queue return 1.
        // while(out_hd_queue(&hdque, &rwinfo))
        // {
        // 	hd_rdwt_real(rwinfo);
        // 	rwinfo->proc->task.stat = READY;
        // }
        out_hd_queue(&hdque, &rwinfo);
        if (rwinfo) {
            hd_rdwt_real(rwinfo);
            proc = rwinfo->proc;
            kern_kfree((u32)rwinfo->msg);
            kern_kfree((u32)rwinfo);
            wakeup(&hdque);
        }

        // kprintf("H ");
        // milli_delay(100);
    }
}

static void hd_rdwt_real(hd_rwinfo_t *p) {
    hd_rdwt(p->msg);
}

void hd_rdwt_sched(hd_messge_t *p) {
    hd_rwinfo_t *rwinfo = (hd_rwinfo_t *)kern_kmalloc(sizeof(hd_rwinfo_t));
    int size = p->CNT;
    UNUSED(size);

    // buffer = (void*)K_PHY2LIN(sys_kmalloc(size));
    // buffer = (void*)K_PHY2LIN(do_kmalloc(size));	//modified by mingxuan
    // 2021-3-25
    // buffer = (void*)kern_kmalloc(size);	//modified by mingxuan 2021-8-16
    rwinfo->msg = p;
    rwinfo->proc = p_proc_current;

    if (p->type == DEV_READ) {
        in_hd_queue(&hdque, rwinfo);
        // phys_copy(p->BUF, buffer, p->CNT);
    } else {
        // phys_copy(buffer, p->BUF, p->CNT);
        in_hd_queue(&hdque, rwinfo);
    }
    wait_event(&hdque);
    // kern_kfree((u32)buffer);
    // kern_kfree((u32)rwinfo);
}

void init_hd_queue(hd_queue_t *hdq) {
    hdq->front = hdq->rear = NULL;
}

static void in_hd_queue(hd_queue_t *hdq, hd_rwinfo_t *p) {
    ksem_wait(&hdque_empty, 1);
    ksem_wait(&hdque_mutex, 1);

    p->next = NULL;
    if (hdq->rear == NULL) { // put in the first node
        hdq->front = hdq->rear = p;
    } else {
        hdq->rear->next = p;
        hdq->rear = p;
    }

    ksem_post(&hdque_mutex, 1);
    ksem_post(&hdque_full, 1);
}

static void out_hd_queue(hd_queue_t *hdq, hd_rwinfo_t **p) {
    ksem_wait(&hdque_full, 1);
    ksem_wait(&hdque_mutex, 1);

    *p = hdq->front;
    if (hdq->front == hdq->rear) { // put out the last node
        hdq->front = hdq->rear = NULL;
    } else {
        hdq->front = hdq->front->next;
    }

    ksem_post(&hdque_mutex, 1);
    ksem_post(&hdque_empty, 1);
}
//~xw

/*****************************************************************************
 *                                hd_ioctl
 *****************************************************************************/
/**
 * <Ring 1> This routine handles the DEV_IOCTL message.
 *
 * @param p  Ptr to the hd_messge_t.
 *****************************************************************************/
void hd_ioctl(hd_messge_t *p) {
    int part_num = p->DEVICE & 0x0FFFFF;
    int drive = DRV_OF_DEV(p->DEVICE);

    struct hd_info *hdi = &hd_infos[drive];

    if (p->REQUEST == DIOCTL_GET_GEO) {
        void *dst = va2la(p->PROC_NR, p->BUF);
        // void * src = va2la(proc2pid(p_proc_current),
        // 		   device < MAX_PRIM ?
        // 		   &hdi->primary[device] :
        // 		   &hdi->logical[(device - MINOR_hd1a) %
        // 				NR_SUB_PER_DRIVE]);

        void *src = va2la(proc2pid(p_proc_current), &hdi->part[part_num]);

        memcpy(dst, src, sizeof(struct part_info));
    } else {
        // assert(0);
    }
}

/*****************************************************************************
 *                                read_part_table_sector
 *****************************************************************************/
/**
 * <Ring 1> Get a partition table of a drive. modify 20240331:
 *统一优化为读取所在的扇区，但是内核栈只有2K尽量减少 内核栈占用
 *
 * @param drive   Drive nr (0 for the 1st disk, 1 for the 2nd, ...)n
 * @param sect_nr The sector at which the partition table is located.
 * @param entry   Ptr to part_ent struct.
 *****************************************************************************/
static void read_part_table_sector(int drive, int nr_sect, void *sect_buf) {
    hd_rdwt_base(drive, DEV_READ, (u32)nr_sect, SECTOR_SIZE, sect_buf);
    return;
}

static int partition_get_fstype(int drive, int start_sect) {
    for (int type = 1; type < NR_FS_TYPE; ++type) {
        if (fstype_table[type].identify) {
            if (fstype_table[type].identify(drive, (u32)start_sect) == 1) { return type; }
        }
    }
    return FS_TYPE_NONE;
}

/*****************************************************************************
 *                                partition
 *****************************************************************************/
/**
 * <Ring 1> This routine is called when a device is opened. It reads the
 * partition table(s) and fills the hd_info struct.
 *
 * @param device Device nr.
 * @param style  P_PRIMARY or P_EXTENDED.
 *****************************************************************************/
static void partition(int device, int style) {
    int i;
    int drive = DRV_OF_DEV(device);
    struct hd_info *hdi = &hd_infos[drive];
    int no_part_count = 0;

    // added by ran
    struct part_info *logical = hdi->part;

    char *sector_buf = (char *)kern_kmalloc(SECTOR_SIZE);
    struct part_ent *part_tbl =
        (struct part_ent *)((char *)sector_buf +
                            PARTITION_TABLE_OFFSET); // first sector at most 4 item

    if (style == P_PRIMARY) {
        read_part_table_sector(drive, 0, sector_buf);
        int nr_prim_parts = 0;
        UNUSED(nr_prim_parts);
        for (i = 0; i < NR_PART_PER_DRIVE; ++i) { /* 0~3 */
            if (part_tbl[i].sys_id == NO_PART) {
                no_part_count++;
                continue;
            }

            nr_prim_parts++;
            int dev_nr = i + 1; /* 1~4 */
            hdi->part[dev_nr].base = part_tbl[i].start_sect;
            hdi->part[dev_nr].size = part_tbl[i].nr_sects;
            hdi->part[dev_nr].fs_type = partition_get_fstype(drive, hdi->part[dev_nr].base);
            // added by mingxuan 2020-10-27
            // struct fs_flags fs_flags_real;
            // struct fs_flags *fs_flags_buf = &fs_flags_real;
            // get_fs_flags(drive,
            // hdi->part[dev_nr].base+BLOCK_SIZE/SECTOR_SIZE, fs_flags_buf);
            // //hdi->primary[dev_nr].base + 1 beacause of orange and fat32 is
            // in 2nd sector, mingxuan if(fs_flags_buf->orange_flag == 0x11)
            // // Orange's Magic 	hdi->part[dev_nr].fs_type = FS_TYPE_ORANGE;

            // comment added by ran 2021-01-15
            // 这里的逻辑是判断分区的1号扇区是否有FAT32文件系统的标记，
            // 可以看出这8个字节是字符串"MSDOS5.0"，可以猜测这段代码的目的是
            // 读取Boot Sector的OEM Name字段，不过Boot Sector应当是
            // 在分区的0号扇区而非第1个扇区，第1个扇区通常是FS Information
            // Sector。 并且使用不同格式化工具会产生不同的OEM Name，比如Linux的
            // 格式化工具mkfs.fat就会将OEM Name字段设置为"mkfs.fat"。
            // 经过查找文档，我找到了判断FAT32文件系统的方法，
            // 在FAT32 Extended BIOS Parameter Block中有一个字段
            // File system type，具体的位置是Boot Sector的0x052字节开始，
            // 共8个字节，通过判断该字段是否为{4641-5433-3220-2020}("FAT32   "),
            // 可以判断当前扇区的文件系统是否为FAT32
            // deleted by ran
            // else if(fs_flags_buf->fat32_flag1 == 0x534f4453 &&
            // fs_flags_buf->fat32_flag2 == 0x302e35) // FAT32 flags
            // 	hdi->primary[dev_nr].fs_type = FS_TYPE_FAT32;
            // added end, mingxuan 2020-10-27

            // added by ran
            // else if (is_fat32_part(drive, hdi->part[dev_nr].base))
            // {
            // 	hdi->part[dev_nr].fs_type = FS_TYPE_FAT32;
            // }

            if (part_tbl[i].sys_id == EXT_PART) /* extended */
                partition(device | dev_nr, P_EXTENDED);
        }
    } else if (style == P_EXTENDED) {
        int j = (device & 0x0FFFFF); /* 1~4 */
        int ext_start_sect = hdi->part[j].base;
        int s = ext_start_sect;
        int nr_1st_sub = 5; /* 扩展分区第一个分区号是5 */

        for (i = 0; i < NR_SUB_PER_PART; ++i) {
            int dev_nr = nr_1st_sub + i; /* 5~*/

            read_part_table_sector(drive, s, sector_buf);

            // hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
            // hdi->logical[dev_nr].size = part_tbl[0].nr_sects;
            // modified by ran
            logical[dev_nr].base = s + part_tbl[0].start_sect;
            logical[dev_nr].size = part_tbl[0].nr_sects;

            // added by ran
            int boot_sec = hdi->part[dev_nr].base;

            logical[dev_nr].fs_type = partition_get_fstype(drive, boot_sec);

            s = ext_start_sect + part_tbl[1].start_sect;

            /* no more logical partitions
               in this extended partition */
            if (part_tbl[1].sys_id == NO_PART) break;
        }
    }
    if (no_part_count == 4) { // 没有分区
        int dev_nr = 0;

        int boot_sec = hdi->part[dev_nr].base;

        logical[dev_nr].fs_type = partition_get_fstype(drive, boot_sec);

        // assert(0);
    }
    kern_kfree((u32)sector_buf);
}

/*****************************************************************************
 *                                hd_identify
 *****************************************************************************/
/**
 * <Ring 1> Get the disk information.
 *
 * @param drive  Drive Nr.
 *****************************************************************************/
static void hd_identify(int drive) {
    hd_cmd_t cmd;
    cmd.device = MAKE_DEVICE_REG(0, drive, 0);
    cmd.command = ATA_IDENTIFY;
    hd_cmd_out(&cmd, drive);
    interrupt_wait();
    port_read(REG_DATA, hdbuf, SECTOR_SIZE);

    // print_identify_info((u16*)hdbuf);	//deleted by mingxuan 2021-2-7

    u16 *hdinfo = (u16 *)hdbuf;

    int cmd_set_supported = hdinfo[83];

    // kprintf("LBA48 supported:");
    if ((cmd_set_supported & 0x0400)) {
        hd_LBA48_sup[drive] = 1;
        // kprintf("YES  ");
    } // by zql 2022.4.26

    hd_infos[drive].part[0].base = 0;
    /* Total Nr of User Addressable Sectors */
    hd_infos[drive].part[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
}

//! defined but not used
/*****************************************************************************
 *                            print_identify_info
 *****************************************************************************/
/**
 * <Ring 1> Print the hdinfo retrieved via ATA_IDENTIFY command.
 *
 * @param hdinfo  The buffer read from the disk i/o port.
 *****************************************************************************/
MAYBE_UNUSED void print_identify_info(u16 *hdinfo) {
    int i;
    char s[64];

    struct iden_info_ascii {
        int idx;
        int len;
        char *desc;
    } iinfo[] = {{10, 20, "HD SN"}, /* Serial number in ASCII */
                 {27, 40, "HD Model"} /* Model number in ASCII */};

    for (size_t k = 0; k < sizeof(iinfo) / sizeof(iinfo[0]); ++k) {
        char *p = (char *)&hdinfo[iinfo[k].idx];
        for (i = 0; i < iinfo[k].len / 2; ++i) {
            s[i * 2 + 1] = *p++;
            s[i * 2] = *p++;
        }
        s[i * 2] = 0;
        // printl("%s: %s\n", iinfo[k].desc, s);
        kprintf(iinfo[k].desc);
        kprintf(":");
        kprintf(s);
        kprintf("\n");
    }

    int capabilities = hdinfo[49];
    // printl("LBA supported: %s\n", (capabilities & 0x0200) ? "Yes" : "No");
    kprintf("LBA supported:");
    if ((capabilities & 0x0200))
        kprintf("YES  ");
    else
        kprintf("NO  ");
    // kprintf("\n");

    int cmd_set_supported = hdinfo[83];
    // printl("LBA48 supported: %s\n", (cmd_set_supported & 0x0400) ? "Yes" :
    // "No");
    kprintf("LBA48 supported: %s", (cmd_set_supported & 0x0400) ? "YES" : "NO");
    kprintf("\n");
}

/*****************************************************************************
 *                                hd_cmd_out
 *****************************************************************************/
/**
 * <Ring 1> Output a command to HD controller.
 *
 * @param cmd  The command struct ptr.
 *****************************************************************************/
static void hd_cmd_out(hd_cmd_t *cmd, int drive) {
    /**
     * For all commands, the host must first check if BSY=1,
     * and should proceed no further unless and until BSY=0
     */
    if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT))
        // panic("hd error.");
        kprintf("hd error.");

    /* Activate the Interrupt Enable (nIEN) bit */
    outb(REG_DEV_CTRL, 0);

    if (hd_LBA48_sup[drive] == 1) { // 若是大硬盘，则LBA48第一次写
        outb(REG_FEATURES, cmd->features);
        outb(REG_NSECTOR, cmd->count_LBA48);
        outb(REG_LBA_LOW, cmd->lba_low_LBA48);
        outb(REG_LBA_MID, cmd->lba_mid_LBA48);
        outb(REG_LBA_HIGH, cmd->lba_high_LBA48);
    } // by qianglong 2022.4.26

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

/*****************************************************************************
 *                                interrupt_wait
 *****************************************************************************/
/**
 * <Ring 1> Wait until a disk interrupt occurs.
 *
 *****************************************************************************/
/// modified by zcr(using Huper's method.)
// PUBLIC void interrupt_wait()
// {
// 	while(hd_int_waiting_flag) {
// 		// milli_delay(20);/// waiting for the harddisk interrupt.
// 	}
// 	hd_int_waiting_flag = 1;
// }

//	/*
static void interrupt_wait() {
    while (hd_int_waiting_flag) {
        // milli_delay invoke syscall getticks, so we can't use it here.
        // for this scene, just do nothing is OK. modified by xw, 18/6/1

        // milli_delay(5);/// waiting for the harddisk interrupt.
    }
    hd_int_waiting_flag = 1;
}
//	*/

/*
//added by xw, 18/8/16
void interrupt_wait_sched()
{
while(hd_int_waiting_flag){
        sched();
}
hd_int_waiting_flag = 1;
}
//	*/

/*****************************************************************************
 *                                waitfor
 *****************************************************************************/
/**
 * <Ring 1> Wait for a certain status.
 *
 * @param mask    Status mask.
 * @param val     Required status.
 * @param timeout Timeout in milliseconds.
 *
 * @return One if sucess, zero if timeout.
 *****************************************************************************/
static int waitfor(int mask, int val, int timeout) {
    int t = kern_getticks(); // modified by mingxuan 2021-8-14

    while (((kern_getticks() - t) * 1000 / HZ) < timeout) { // modified by mingxuan 2021-8-14
        if ((inb(REG_STATUS) & mask) == val) return 1;
    }

    return 0;
}

/*****************************************************************************
 *                                hd_handler
 *****************************************************************************/
/**
 * <Ring 0> Interrupt handler.
 *
 * @param irq  IRQ nr of the disk interrupt.
 *****************************************************************************/
static void hd_handler(int irq) {
    UNUSED(irq);
    /*
     * Interrupts are cleared when the host
     *   - reads the Status Register,
     *   - issues a reset, or
     *   - writes to the Command Register.
     */
    hd_status = inb(REG_STATUS);
    inform_int();

    /* There is two stages - in kernel intializing or in process running.
     * Some operation shouldn't be valid in kernel intializing stage.
     * added by xw, 18/6/1
     */
    if (kstate_on_init) { return; }

    // some operation only for process

    return;
}

/*****************************************************************************
 *                                inform_int
 *****************************************************************************/
static void inform_int() {
    hd_int_waiting_flag = 0;
    return;
}

/*
 * 读写IDE
 */
static void IDE_rdwt(int drive, int type, u64 sect_nr, u32 count, void *buf) {
    hd_cmd_t cmd;
    u32 n = (count + SECTOR_SIZE - 1) / SECTOR_SIZE;
    cmd.features = 0;
    // cmd.count	= (p->msg->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
    cmd.count = n & 0xFF;
    cmd.lba_low = sect_nr & 0xFF;
    cmd.lba_mid = (sect_nr >> 8) & 0xFF;
    cmd.lba_high = (sect_nr >> 16) & 0xFF;

    if (hd_LBA48_sup[drive] == 1) { // LBA48
        cmd.count_LBA48 = (n >> 8) & 0xFF;
        cmd.lba_low_LBA48 = (sect_nr >> 24) & 0xFF;
        cmd.lba_mid_LBA48 = (sect_nr >> 32) & 0xFF;
        cmd.lba_high_LBA48 = (sect_nr >> 40) & 0xFF;
        cmd.device =
            0x40 | ((drive << 4) & 0xFF); // 0~3位,0；第4位0表示主盘,1表示从盘；7~5位,010,表示为LBA
        cmd.command = (type == DEV_READ) ? ATA_READ_EXT : ATA_WRITE_EXT;
    } // by qianglong 2022.4.26
    else { // LBA28
        cmd.device = MAKE_DEVICE_REG(1, drive, (sect_nr >> 24) & 0xF);
        cmd.command = (type == DEV_READ) ? ATA_READ : ATA_WRITE;
    }
    hd_cmd_out(&cmd, drive);

    int bytes_left = count;
    char *la = buf; // attention here!

    while (bytes_left) {
        int bytes = MIN(SECTOR_SIZE, bytes_left);
        if (type == DEV_READ) {
            interrupt_wait();
            port_read(REG_DATA, hdbuf, SECTOR_SIZE);
            memcpy(la, hdbuf, bytes);
        } else {
            if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT)) kprintf("hd writing error.");

            memcpy(hdbuf, la, bytes);
            port_write(REG_DATA, hdbuf, SECTOR_SIZE);
            interrupt_wait();
        }
        bytes_left -= bytes;
        la += bytes;
    }
}

static int SATA_rdwt(int drive, int type, u64 sect_nr, u32 count, void *buf) {
    // int drive = DRV_OF_DEV(p->DEVICE);

    // u64 pos = p->POSITION;
    // u64	sect_nr = (pos >> SECTOR_SIZE_SHIFT);

    // sect_nr += hd_infos[drive].part[p->DEVICE & 0x0FFFFF].base;

    u32 sector_count = (count + SECTOR_SIZE - 1) / SECTOR_SIZE;

    if (type == DEV_WRITE) { memcpy(satabuf, buf, count); }

    SATA_rdwt_sects(drive, type, sect_nr, sector_count);

    if (type == DEV_READ) { memcpy(buf, satabuf, count); }

    return 1;
}

/*
 * @brief 读/写SATA硬盘
 * @param drive		drive统一编码
 * @param type 		0: read, device to host ;	1: write, host to device
 * @param sect_nr	读/写的起始扇区
 * @param count 	读/写扇区的数量
 * @retval			0: error  ;  1:sucess
 * @note			还没实现出错重发的功能;"satabuf +=
 * 8*1024;"这行存在bug;
 */
static int SATA_rdwt_sects(int drive, int type, u64 sect_nr, u32 count) {
    if (drive < SATA_BASE || drive >= SATA_LIMIT) {
        kprintf("ERROR:SATA_rdwt_sects");
        return FALSE;
    }
    drive -= SATA_BASE;

    int port_num = ahci_info[0].satadrv_atport[drive];
    HBA_PORT *port = &(HBA->ports[port_num]);
    port->is = (u32)-1; // Clear pending interrupt bits

    int slot = find_cmdslot(port);
    if (slot == -1) return FALSE;

    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER *)K_PHY2LIN(port->clb);

    cmdheader += slot;
    // memset(cmdheader, 0, sizeof(HBA_CMD_HEADER));
    cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(u32); // Command FIS size
    cmdheader->w = (type == DEV_READ) ? 0 : 1; // 0:device to host,read ;1:host to device,write
    cmdheader->c = 0;
    cmdheader->p = 0; // Software shall not set CH(pFreeSlot).P when building
                      // queued ATA commands.
    cmdheader->prdbc = 0;
    cmdheader->prdtl = 1; // PRDT entries count

    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL *)K_PHY2LIN(cmdheader->ctba);
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL) + (cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

    int i = 0;
    for (i = 0; i < cmdheader->prdtl - 1; ++i) {
        cmdtbl->prdt_entry[i].dba = (u32)(K_LIN2PHY(satabuf));
        cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1; // 8K bytes (this value should always be set to 1
                                                  // less than the actual value)
        cmdtbl->prdt_entry[i].i = 0;
        satabuf += 8 * 1024; // 8k bytes
        count -= 16;         // 16 sectors
    }
    // Last entry202.117.249.6
    cmdtbl->prdt_entry[i].dba = (u32)(K_LIN2PHY(satabuf));
    cmdtbl->prdt_entry[i].dbc = (count << 9) - 1; // 512 bytes per sector
    cmdtbl->prdt_entry[i].i = 0;

    // Setup command
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D *)(&cmdtbl->cfis);

    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1; // Command

    // cmdfis->command = (p->type == DEV_READ) ? ATA_CMD_READ_DMA_EX :
    // ATA_CMD_WRITE_DMA_EX;
    cmdfis->command = (type == DEV_READ) ? ATA_CMD_READ_DMA_EX : ATA_CMD_WRITE_DMA_EX;

    cmdfis->lba0 = sect_nr & 0xFF;
    cmdfis->lba1 = (sect_nr >> 8) & 0xFF;
    cmdfis->lba2 = (sect_nr >> 16) & 0xFF;
    cmdfis->device = 1 << 6; // LBA mode

    cmdfis->lba3 = (sect_nr >> 24) & 0xFF;
    cmdfis->lba4 = (sect_nr >> 32) & 0xFF;
    cmdfis->lba5 = (sect_nr >> 40) & 0xFF;

    cmdfis->countl = count & 0xFF;
    cmdfis->counth = (count >> 8) & 0xFF;

    // The below loop waits until the port is no longer busy before issuing a
    // new command
    int spin = 0; // Spin lock timeout counter
    while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000) { spin++; }
    if (spin == 1000000) {
        kprintf("\nPort is hung");
        return FALSE;
    }

    if (kstate_on_init) {
        port->ci = 1 << slot; // Issue command
        while (sata_wait_flag);
        sata_wait_flag = 1;
    } else {
        /*此处采用开关中断的设计是为了防止sata中断在将hd_service设置为SLEEPING前到来*/
        disable_int_begin();
        port->ci = 1 << slot; // Issue command
        wait_event((void *)&sata_wait_flag);
        disable_int_end();
    }

    if (sata_error_flag == 1)
        return FALSE;
    else
        return TRUE;
}

// add by sundong 2023.5.26
int rw_blocks(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf) {
    hd_messge_t *driver_msg = (hd_messge_t *)kern_kmalloc(sizeof(hd_messge_t));
    driver_msg->type = io_type;
    // driver_msg.DEVICE	= MINOR(dev);
    driver_msg->DEVICE = dev;

    driver_msg->POSITION = pos;
    driver_msg->CNT = bytes;
    driver_msg->PROC_NR = proc_nr;
    driver_msg->BUF = buf;

    // assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
    // send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &driver_msg);

    /// replace the statement above.
    hd_rdwt(driver_msg);
    // kern_kfree((u32)driver_msg);
    return 0;
}

// add by sundong 2023.5.26
int rw_blocks_sched(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf) {
    // driver_msg will be used in hd_service(another process) why use stack
    // memory??? jiangfeng 20240323 hd_messge_t driver_msg;
    hd_messge_t *driver_msg = (hd_messge_t *)kern_kmalloc(sizeof(hd_messge_t));
    driver_msg->type = io_type;
    // driver_msg.DEVICE	= MINOR(dev);
    driver_msg->DEVICE = dev;

    driver_msg->POSITION = pos;
    driver_msg->CNT = bytes;
    driver_msg->PROC_NR = proc_nr;
    driver_msg->BUF = buf;

    hd_rdwt_sched(driver_msg);
    // kern_kfree((u32)driver_msg);

    return 0;
}

// added by mingxuan 2020-10-27
// refact 20240328
int orangefs_identify(int drive, u32 nr_sect) {
    struct fs_flags fs_flags_real;
    hd_rdwt_base(drive, DEV_READ, (u32)(nr_sect + 8), sizeof(struct fs_flags), &fs_flags_real);
    return fs_flags_real.orange_flag == 0x11;
}

// added by ran
// is_fat32_part函数的功能是判断分区是否为FAT32文件系统
int fat32_identify(int drive, u32 nr_sect) {
    char sect_buf[SECTOR_SIZE];
    hd_rdwt_base(drive, DEV_READ, nr_sect, SECTOR_SIZE, sect_buf);
    const char *fat32_flag = (void *)(sect_buf + 0x52);
    return strncmp(fat32_flag, "FAT32   ", 8) == 0;
}

int ext4_identify(int drive, u32 nr_sect) {
    char sect_buf[SECTOR_SIZE];
    unsigned short magic;
    // 这里要 +2 因为前 1024 字节是填充 0
    hd_rdwt_base(drive, DEV_READ, nr_sect + 2, SECTOR_SIZE, sect_buf);

    magic = *(unsigned short *)(sect_buf + 0x38);

    return magic == 0xef53;
}
