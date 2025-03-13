#pragma once

#include <minios/proc.h>
#include <klib/stdint.h>

struct mess1 {
    int m1i1;
    int m1i2;
    int m1i3;
    int m1i4;
};

struct mess2 {
    void *m2p1;
    void *m2p2;
    void *m2p3;
    void *m2p4;
};

struct mess3 {
    int m3i1;
    int m3i2;
    int m3i3;
    int m3i4;
    u64 m3l1;
    u64 m3l2;
    void *m3p1;
    void *m3p2;
};

typedef struct {
    int source;
    int type;
    union {
        struct mess1 m1;
        struct mess2 m2;
        struct mess3 m3;
    } u;
} hd_messge_t;

/**
 * @enum msgtype
 * @brief hd_messge_t types
 */
enum msgtype {
    /*
     * when hard interrupt occurs, a msg (with type==HARD_INT) will
     * be sent to some tasks
     */
    HARD_INT = 1,

    /* SYS task */
    GET_TICKS,

    /// zcr added from ch9/e/include/const.h
    /* FS */
    OPEN,
    CLOSE,
    READ,
    WRITE,
    LSEEK,
    STAT,
    UNLINK,
    CREATEDIR,
    OPENDIR,
    DELETEDIR,
    SHOWDIR,

    /* message type for drivers */
    DEV_OPEN = 1001,
    DEV_CLOSE,
    DEV_READ,
    DEV_WRITE,
    DEV_IOCTL
};

/**
 * @struct part_ent
 * @brief  Partition Entry struct.
 *
 * <b>Master Boot Record (MBR):</b>
 *   Located at offset 0x1be in the 1st sector of a disk. MBR contains
 *   four 16-byte partition entries. Should end with 55h & AAh.
 *
 * <b>partitions in MBR:</b>
 *   A PC hard disk can contain either as many as four primary partitions,
 *   or 1-3 primaries and a single extended partition. Each of these
 *   partitions are described by a 16-byte entry in the Partition Table
 *   which is located in the Master Boot Record.
 *
 * <b>extented partition:</b>
 *   It is essentially a link list with many tricks. See
 *   http://en.wikipedia.org/wiki/Extended_boot_record for details.
 */
struct part_ent {
    u8 boot_ind; /**
                  * boot indicator
                  *   Bit 7 is the active partition flag,
                  *   bits 6-0 are zero (when not zero this
                  *   byte is also the drive number of the
                  *   drive to boot so the active partition
                  *   is always found on drive 80H, the first
                  *   hard disk).
                  */

    u8 start_head; /**
                    * Starting Head
                    */

    u8 start_sector; /**
                      * Starting Sector.
                      *   Only bits 0-5 are used. Bits 6-7 are
                      *   the upper two bits for the Starting
                      *   Cylinder field.
                      */

    u8 start_cyl; /**
                   * Starting Cylinder.
                   *   This field contains the lower 8 bits
                   *   of the cylinder value. Starting cylinder
                   *   is thus a 10-bit number, with a maximum
                   *   value of 1023.
                   */

    u8 sys_id; /**
                * System ID
                * e.g.
                *   01: FAT12
                *   81: MINIX
                *   83: Linux
                */

    u8 end_head; /**
                  * Ending Head
                  */

    u8 end_sector; /**
                    * Ending Sector.
                    *   Only bits 0-5 are used. Bits 6-7 are
                    *   the upper two bits for the Ending
                    *    Cylinder field.
                    */

    u8 end_cyl; /**
                 * Ending Cylinder.
                 *   This field contains the lower 8 bits
                 *   of the cylinder value. Ending cylinder
                 *   is thus a 10-bit number, with a maximum
                 *   value of 1023.
                 */

    u32 start_sect; /**
                     * starting sector counting from
                     * 0 / Relative Sector. / start in LBA
                     */

    u32 nr_sects; /**
                   * nr of sectors in partition
                   */

}; // PARTITION_ENTRY;

struct fs_flags {
    u8 orange_flag;
    u16 reserved;
    u32 fat32_flag1;
    u32 fat32_flag2;
};

/********************************************/
/* I/O Ports used by hard disk controllers. */
/********************************************/
/* slave disk not supported yet, all master registers below */

/* Command Block Registers */
/*	MACRO		PORT			DESCRIPTION
 * INPUT/OUTPUT	*/
/*	-----		----			-----------
 * ------------	*/
#define REG_DATA 0x1f0         /*	Data				I/O		*/
#define REG_FEATURES 0x1f1     /*	Features			O		*/
#define REG_ERROR REG_FEATURES /*	Error				I		*/
/* 	The contents of this register are valid only when the error bit
        (ERR) in the Status Register is set, except at drive power-up or at the
        completion of the drive's internal diagnostics, when the register
        contains a status code.
        When the error bit (ERR) is set, Error Register bits are interpreted as
   such: |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
        +-----+-----+-----+-----+-----+-----+-----+-----+
        | BRK | UNC |     | IDNF|     | ABRT|TKONF| AMNF|
        +-----+-----+-----+-----+-----+-----+-----+-----+
           |     |     |     |     |     |     |     |
           |     |     |     |     |     |     |     `--- 0. Data address mark
   not found after correct ID field found |     |     |     |     |     |
   `--------- 1. Track 0 not found during execution of Recalibrate command | |
   |     |     |     `--------------- 2. Command aborted due to drive status
   error or invalid command |     |     |     |     `--------------------- 3.
   Not used |     |     |     `--------------------------- 4. Requested sector's
   ID field not found |     |     `--------------------------------- 5. Not used
           |     `--------------------------------------- 6. Uncorrectable data
   error encountered
           `--------------------------------------------- 7. Bad block mark
   detected in the requested sector's ID field
*/
#define REG_NSECTOR 0x1f2  /*	Sector Count			I/O		*/
#define REG_LBA_LOW 0x1f3  /*	Sector Number / LBA Bits 0-7	I/O		*/
#define REG_LBA_MID 0x1f4  /*	Cylinder Low / LBA Bits 8-15	I/O		*/
#define REG_LBA_HIGH 0x1f5 /*	Cylinder High / LBA Bits 16-23	I/O		*/
#define REG_DEVICE                             \
    0x1f6 /*	Drive | Head | LBA bits 24-27	I/O \
           */
          /*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
                  +-----+-----+-----+-----+-----+-----+-----+-----+
                  |  1  |  L  |  1  | DRV | HS3 | HS2 | HS1 | HS0 |
                  +-----+-----+-----+-----+-----+-----+-----+-----+
                           |           |   \_____________________/
                           |           |              |
                           |           |              `------------ If L=0, Head Select.
                           |           |                                    These four
             bits select the head number.         |           |         HS0 is the least
             significant.         |         |         If L=1, HS0 through HS3 contain bit
             24-27 of         the LBA.         |
             `--------------------------- Drive. When DRV=0, drive 0 (master) is selected.
                           |                                               When DRV=1,
             drive 1 (slave) is selected.
                           `--------------------------------------- LBA mode. This bit
             selects the mode of operation.         When L=0, addressing is by 'CHS' mode.
             When         L=1, addressing is by 'LBA' mode.
          */
#define REG_STATUS       \
    0x1f7 /*	Status				I \
           */
/* 	Any pending interrupt is cleared whenever this register is read.
        |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
        +-----+-----+-----+-----+-----+-----+-----+-----+
        | BSY | DRDY|DF/SE|  #  | DRQ |     |     | ERR |
        +-----+-----+-----+-----+-----+-----+-----+-----+
           |     |     |     |     |     |     |     |
           |     |     |     |     |     |     |     `--- 0. Error.(an error
   occurred)         |     |     |     |     |     |     `--------- 1. Obsolete.
   |     |         |     |     |     `--------------- 2. Obsolete.         | | |
   |
   `--------------------- 3. Data Request. (ready to transfer data)         | |
   |
   `--------------------------- 4. Command dependent. (formerly DSC bit) |     |
   `--------------------------------- 5. Device Fault / Stream Error.         |
   `--------------------------------------- 6. Drive Ready.
           `--------------------------------------------- 7. Busy. If BSY=1, no
   other bits in the register are valid.
*/
#define STATUS_BSY 0x80
#define STATUS_DRDY 0x40
#define STATUS_DFSE 0x20
#define STATUS_DSC 0x10
#define STATUS_DRQ 0x08
#define STATUS_CORR 0x04
#define STATUS_IDX 0x02
#define STATUS_ERR 0x01

#define REG_CMD REG_STATUS
/*!
 *    +--------+---------------------------------+-----------------+
 *    | Command| Command Description             | Parameters Used |
 *    | Code   |                                 | PC SC SN CY DH  |
 *    +--------+---------------------------------+-----------------+
 *    | ECh  @ | Identify Drive                  |             D   |
 *    | 91h    | Initialize Drive Parameters     |    V        V   |
 *    | 20h    | Read Sectors With Retry         |    V  V  V  V   |
 *    | E8h  @ | Write Buffer                    |             D   |
 *    +--------+---------------------------------+-----------------+
 *
 *    KEY FOR SYMBOLS IN THE TABLE:
 *    PC    Register 1F1: Write Precompensation
 *    @     These commands are optional and may not be supported by some drives.
 *    SC    Register
 *    1F2:  Sector Count
 *    D     Only DRIVE parameter is valid, HEAD parameter is ignored.
 *    SN    Register 1F3: Sector Number
 *    D+    Both drives execute this command regardless of the DRIVE parameter.
 *    CY    Register
 *    1F4+1F5  Cylinder low + high
 *    V     Indicates that the register contains a valid paramterer.
 *    DH    Register 1F6: Drive / Head
 */

/* Control Block Registers */
/*	MACRO		PORT			DESCRIPTION
 * INPUT/OUTPUT	*/
/*	-----		----			-----------
 * ------------	*/
#define REG_DEV_CTRL 0x3f6
/*	Device Control			O		*/
/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
        +-----+-----+-----+-----+-----+-----+-----+-----+
        | HOB |  -  |  -  |  -  |  -  |SRST |-IEN |  0  |
        +-----+-----+-----+-----+-----+-----+-----+-----+
           |                             |     |
           |                             |     `--------- Interrupt Enable.
           |                             |                  - IEN=0, and the
   drive is selected,         |                             |         drive
   interrupts         to the host will be enabled.         |         |         -
   IEN=1,         or the drive is not selected,         |         |                            drive
   interrupts to         the host will be disabled.         | `--------------- Software Reset. | -
   The drive         is held         reset when RST=1.         |         Setting
   RST=0         re-enables         the         drive.         |         - The host must set RST=1
   and         wait         for at         least                            | 5 microsecondsbefore
   setting         RST=0,         to         ensure                            | that the drive
   recognizes         the         reset.
           `--------------------------------------------- HOB (High Order Byte)
                                                            - defined by 48-bit
   Address feature set.
*/
#define REG_ALT_STATUS REG_DEV_CTRL /*	Alternate Status		I		*/
/*	This register contains the same information as the Status Register.
        The only difference is that reading this register does not imply
   interrupt acknowledge or clear a pending interrupt.
*/

#define REG_DRV_ADDR 0x3f7 /*	Drive Address			I		*/

typedef struct hd_cmd {
    u8 features;
    u8 count;
    u8 count_LBA48; // LBA48,要读写的扇区数的高8位,LBA48模式下写两次端口寄存器，先写高位,地址也是先写高位
                    // by qianglong 2022.4.25
    u8 lba_low;
    u8 lba_low_LBA48; // LBA48,24~31位
    u8 lba_mid;
    u8 lba_mid_LBA48; // LBA48,32~39位
    u8 lba_high;
    u8 lba_high_LBA48; // LBA48,40~47位
    u8 device;
    u8 command;
} hd_cmd_t;

/* macros for messages */
#define FD u.m3.m3i1
#define PATHNAME u.m3.m3p1
#define FLAGS u.m3.m3i1
#define NAME_LEN u.m3.m3i2
#define CNT u.m3.m3i2
#define REQUEST u.m3.m3i2
#define PROC_NR u.m3.m3i3
#define DEVICE u.m3.m3i4
#define POSITION u.m3.m3l1
#define BUF u.m3.m3p2
#define OFFSET u.m3.m3i2
#define WHENCE u.m3.m3i3
#define RETVAL u.m3.m3i1

#define DIOCTL_GET_GEO 1

#define MAX_DRIVES 2
#define NR_PART_PER_DRIVE 4 // 每块硬盘(驱动器)只能有4个主分区, mingxuan
#define NR_SUB_PER_PART 11  // 扩展分区最多有11个逻辑分区, mingxuan
// 每块硬盘(驱动器)最多有16 * 4 = 64个逻辑分区, mingxuan
#define NR_SUB_PER_DRIVE (NR_SUB_PER_PART * NR_PART_PER_DRIVE)
// 表示的是hd[0～4]这5个分区，因为有些代码中我们把整块硬盘（hd0）和主分区（hd[1～4]）放在一起看待,
// mingxuan
#define NR_PRIM_PER_DRIVE (NR_PART_PER_DRIVE + 1)

/**
 * @def MAX_PRIM
 * Defines the max minor number of the primary partitions.
 * If there are 2 disks, prim_dev ranges in hd[0-9], this macro will
 * equals 9.
 */
// MAX_PRIM定义的是主分区的最大值，比如若有两块硬盘，那第一块硬盘的主分区为hd[1～4]，第二块硬盘的主分区为hd[6～9]
// 所以MAX_PRIM为9，我们定义的hd1a的设备号应大于它，这样通过与MAX_PRIM比较，我们就可以知道一个设备是主分区还是逻辑分区
// mingxuan
#define MAX_PRIM (MAX_DRIVES * NR_PRIM_PER_DRIVE - 1)     // MAX_PRIM = 2 * 4 - 1 = 9
#define MAX_SUBPARTITIONS (NR_SUB_PER_DRIVE * MAX_DRIVES) // MAX_SUBPARTITIONS = 64 * 2 = 128

#define NO_PART 0x00  /* unused entry */
#define EXT_PART 0x05 /* extended partition */

struct part_info {
    u32 base;    /* # of start sector (NOT byte offset, but SECTOR) */
    u32 size;    /* how many sectors in this partition */
    int fs_type; // added by mingxuan 2020-10-27
};

/* main drive struct, one entry per drive */
struct hd_info {
    int open_cnt;
    struct part_info part[NR_PRIM_PER_DRIVE + NR_SUB_PER_PART];
};

#define HD_TIMEOUT 10000 /* in millisec */
#define PARTITION_TABLE_OFFSET 0x1be
#define ATA_IDENTIFY 0xec
#define ATA_READ 0x20     // LBA24
#define ATA_READ_EXT 0x24 // LBA48,by qianglong 2022.4.25
#define ATA_WRITE 0x30
#define ATA_WRITE_EXT 0x34 // LBA48,by qianglong 2022.4.25

/* for DEVICE register. */
#define MAKE_DEVICE_REG(lba, drv, lba_highest) \
    (((lba) << 6) | ((drv) << 4) | (lba_highest & 0xf) | 0xa0)

#define HDQUE_MAXSIZE 64

typedef struct hd_rdwt_info {
    hd_messge_t *msg;
    process_t *proc;
    struct hd_rdwt_info *next;
} hd_rwinfo_t;

typedef struct {
    hd_rwinfo_t *front;
    hd_rwinfo_t *rear;
} hd_queue_t;

extern struct hd_info hd_infos[12];

void init_hd();
void hd_open_and_init();

void hd_service();

int hd_make_part_dev(int drive, int part, int fs_type);
int hd_find_dev_of_fstype(int drive, int fs_type);
int hd_get_fstype(int dev);

void hd_open(int drive);
void hd_close(int drive);
void hd_ioctl(hd_messge_t *p);
void hd_rdwt(hd_messge_t *p);
void hd_rdwt_sched(hd_messge_t *p);

void rw_blocks(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf);
void rw_blocks_sched(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf);

int orangefs_identify(int drive, u32 nr_sect);
int fat32_identify(int drive, u32 nr_sect);
int ext4_identify(int drive, u32 nr_sect);
