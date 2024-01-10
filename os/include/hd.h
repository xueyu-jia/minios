/*************************************************************************//**
 *****************************************************************************
 * @file   include/sys/hd.h
 * @brief  
 * @author Forrest Y. Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/

#ifndef	_ORANGES_HD_H_
#define	_ORANGES_HD_H_
#include "type.h"
#include "const.h"
#include "fs_const.h"
#include "proc.h"
/**
 * @struct part_ent
 * @brief  Partition Entry struct.
 *
 * <b>Master Boot Record (MBR):</b>
 *   Located at offset 0x1BE in the 1st sector of a disk. MBR contains
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
	u8 boot_ind;		/**
				 * boot indicator
				 *   Bit 7 is the active partition flag,
				 *   bits 6-0 are zero (when not zero this
				 *   byte is also the drive number of the
				 *   drive to boot so the active partition
				 *   is always found on drive 80H, the first
				 *   hard disk).
				 */

	u8 start_head;		/**
				 * Starting Head
				 */

	u8 start_sector;	/**
				 * Starting Sector.
				 *   Only bits 0-5 are used. Bits 6-7 are
				 *   the upper two bits for the Starting
				 *   Cylinder field.
				 */

	u8 start_cyl;		/**
				 * Starting Cylinder.
				 *   This field contains the lower 8 bits
				 *   of the cylinder value. Starting cylinder
				 *   is thus a 10-bit number, with a maximum
				 *   value of 1023.
				 */

	u8 sys_id;		/**
				 * System ID
				 * e.g.
				 *   01: FAT12
				 *   81: MINIX
				 *   83: Linux
				 */

	u8 end_head;		/**
				 * Ending Head
				 */

	u8 end_sector;		/**
				 * Ending Sector.
				 *   Only bits 0-5 are used. Bits 6-7 are
				 *   the upper two bits for the Ending
				 *    Cylinder field.
				 */

	u8 end_cyl;		/**
				 * Ending Cylinder.
				 *   This field contains the lower 8 bits
				 *   of the cylinder value. Ending cylinder
				 *   is thus a 10-bit number, with a maximum
				 *   value of 1023.
				 */

	u32 start_sect;	/**
				 * starting sector counting from
				 * 0 / Relative Sector. / start in LBA
				 */

	u32 nr_sects;		/**
				 * nr of sectors in partition
				 */

}; //PARTITION_ENTRY;

// added by mingxuan 2020-10-27
struct fs_flags
{
	u8 orange_flag;
	//u8 reserved1;
	//u8 reserved2;
	u16 reserved;
	u32 fat32_flag1;
	u32 fat32_flag2;
};


/********************************************/
/* I/O Ports used by hard disk controllers. */
/********************************************/
/* slave disk not supported yet, all master registers below */

/* Command Block Registers */
/*	MACRO		PORT			DESCRIPTION			INPUT/OUTPUT	*/
/*	-----		----			-----------			------------	*/
#define REG_DATA	0x1F0		/*	Data				I/O		*/
#define REG_FEATURES	0x1F1		/*	Features			O		*/
#define REG_ERROR	REG_FEATURES	/*	Error				I		*/
					/* 	The contents of this register are valid only when the error bit
						(ERR) in the Status Register is set, except at drive power-up or at the
						completion of the drive's internal diagnostics, when the register
						contains a status code.
						When the error bit (ERR) is set, Error Register bits are interpreted as such:
						|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						| BRK | UNC |     | IDNF|     | ABRT|TKONF| AMNF|
						+-----+-----+-----+-----+-----+-----+-----+-----+
						   |     |     |     |     |     |     |     |
						   |     |     |     |     |     |     |     `--- 0. Data address mark not found after correct ID field found
						   |     |     |     |     |     |     `--------- 1. Track 0 not found during execution of Recalibrate command
						   |     |     |     |     |     `--------------- 2. Command aborted due to drive status error or invalid command
						   |     |     |     |     `--------------------- 3. Not used
						   |     |     |     `--------------------------- 4. Requested sector's ID field not found
						   |     |     `--------------------------------- 5. Not used
						   |     `--------------------------------------- 6. Uncorrectable data error encountered
						   `--------------------------------------------- 7. Bad block mark detected in the requested sector's ID field
					*/
#define REG_NSECTOR	0x1F2		/*	Sector Count			I/O		*/
#define REG_LBA_LOW	0x1F3		/*	Sector Number / LBA Bits 0-7	I/O		*/
#define REG_LBA_MID	0x1F4		/*	Cylinder Low / LBA Bits 8-15	I/O		*/
#define REG_LBA_HIGH	0x1F5		/*	Cylinder High / LBA Bits 16-23	I/O		*/
#define REG_DEVICE	0x1F6		/*	Drive | Head | LBA bits 24-27	I/O		*/
					/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						|  1  |  L  |  1  | DRV | HS3 | HS2 | HS1 | HS0 |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						         |           |   \_____________________/
						         |           |              |
						         |           |              `------------ If L=0, Head Select.
						         |           |                                    These four bits select the head number.
						         |           |                                    HS0 is the least significant.
						         |           |                            If L=1, HS0 through HS3 contain bit 24-27 of the LBA.
						         |           `--------------------------- Drive. When DRV=0, drive 0 (master) is selected. 
						         |                                               When DRV=1, drive 1 (slave) is selected.
						         `--------------------------------------- LBA mode. This bit selects the mode of operation.
					 	                                                            When L=0, addressing is by 'CHS' mode.
					 	                                                            When L=1, addressing is by 'LBA' mode.
					*/
#define REG_STATUS	0x1F7		/*	Status				I		*/
					/* 	Any pending interrupt is cleared whenever this register is read.
						|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						| BSY | DRDY|DF/SE|  #  | DRQ |     |     | ERR |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						   |     |     |     |     |     |     |     |
						   |     |     |     |     |     |     |     `--- 0. Error.(an error occurred)
						   |     |     |     |     |     |     `--------- 1. Obsolete.
						   |     |     |     |     |     `--------------- 2. Obsolete.
						   |     |     |     |     `--------------------- 3. Data Request. (ready to transfer data)
						   |     |     |     `--------------------------- 4. Command dependent. (formerly DSC bit)
						   |     |     `--------------------------------- 5. Device Fault / Stream Error.
						   |     `--------------------------------------- 6. Drive Ready.
						   `--------------------------------------------- 7. Busy. If BSY=1, no other bits in the register are valid.
					*/
#define	STATUS_BSY	0x80
#define	STATUS_DRDY	0x40
#define	STATUS_DFSE	0x20
#define	STATUS_DSC	0x10
#define	STATUS_DRQ	0x08
#define	STATUS_CORR	0x04
#define	STATUS_IDX	0x02
#define	STATUS_ERR	0x01

#define REG_CMD		REG_STATUS	/*	Command				O		*/
					/*
						+--------+---------------------------------+-----------------+
						| Command| Command Description             | Parameters Used |
						| Code   |                                 | PC SC SN CY DH  |
						+--------+---------------------------------+-----------------+
						| ECh  @ | Identify Drive                  |             D   |
						| 91h    | Initialize Drive Parameters     |    V        V   |
						| 20h    | Read Sectors With Retry         |    V  V  V  V   |
						| E8h  @ | Write Buffer                    |             D   |
						+--------+---------------------------------+-----------------+
					
						KEY FOR SYMBOLS IN THE TABLE:
						===========================================-----=========================================================================
						PC    Register 1F1: Write Precompensation	@     These commands are optional and may not be supported by some drives.
						SC    Register 1F2: Sector Count		D     Only DRIVE parameter is valid, HEAD parameter is ignored.
						SN    Register 1F3: Sector Number		D+    Both drives execute this command regardless of the DRIVE parameter.
						CY    Register 1F4+1F5: Cylinder low + high	V     Indicates that the register contains a valid paramterer.
						DH    Register 1F6: Drive / Head
					*/

/* Control Block Registers */
/*	MACRO		PORT			DESCRIPTION			INPUT/OUTPUT	*/
/*	-----		----			-----------			------------	*/
#define REG_DEV_CTRL	0x3F6		/*	Device Control			O		*/
					/*	|  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						| HOB |  -  |  -  |  -  |  -  |SRST |-IEN |  0  |
						+-----+-----+-----+-----+-----+-----+-----+-----+
						   |                             |     |
						   |                             |     `--------- Interrupt Enable.
						   |                             |                  - IEN=0, and the drive is selected,
						   |                             |                    drive interrupts to the host will be enabled.
						   |                             |                  - IEN=1, or the drive is not selected,
						   |                             |                    drive interrupts to the host will be disabled.
						   |                             `--------------- Software Reset.
						   |                                                - The drive is held reset when RST=1.
						   |                                                  Setting RST=0 re-enables the drive.
						   |                                                - The host must set RST=1 and wait for at least
						   |                                                  5 microsecondsbefore setting RST=0, to ensure
						   |                                                  that the drive recognizes the reset.
						   `--------------------------------------------- HOB (High Order Byte)
						                                                    - defined by 48-bit Address feature set.
					*/
#define REG_ALT_STATUS	REG_DEV_CTRL	/*	Alternate Status		I		*/
					/*	This register contains the same information as the Status Register.
						The only difference is that reading this register does not imply interrupt acknowledge or clear a pending interrupt.
					*/

#define REG_DRV_ADDR	0x3F7		/*	Drive Address			I		*/


struct hd_cmd {
	u8	features;
	u8	count;
	u8	count_LBA48;//LBA48,要读写的扇区数的高8位,LBA48模式下写两次端口寄存器，先写高位,地址也是先写高位 by qianglong 2022.4.25
	u8	lba_low;
	u8	lba_low_LBA48;//LBA48,24~31位
	u8	lba_mid;
	u8	lba_mid_LBA48;//LBA48,32~39位
	u8	lba_high;
	u8 	lba_high_LBA48;//LBA48,40~47位
	u8	device;
	u8	command;
};

/* macros for messages */
#define	FD			u.m3.m3i1 
#define	PATHNAME	u.m3.m3p1 
#define	FLAGS		u.m3.m3i1 
#define	NAME_LEN	u.m3.m3i2 
#define	CNT			u.m3.m3i2
#define	REQUEST		u.m3.m3i2
#define	PROC_NR		u.m3.m3i3
#define	DEVICE		u.m3.m3i4
#define	POSITION	u.m3.m3l1
#define	BUF			u.m3.m3p2
#define	OFFSET		u.m3.m3i2 
#define	WHENCE		u.m3.m3i3 

/* #define	PID		u.m3.m3i2 */
/* #define	STATUS		u.m3.m3i1 */
#define	RETVAL		u.m3.m3i1
/* #define	STATUS		u.m3.m3i1 */


#define	DIOCTL_GET_GEO	1
/* Hard Drive */
#define SECTOR_SIZE		512
#define SECTOR_BITS		(SECTOR_SIZE * 8)
#define SECTOR_SIZE_SHIFT	9

#define	MAX_DRIVES			2
#define	NR_PART_PER_DRIVE	4	// 每块硬盘(驱动器)只能有4个主分区, mingxuan
#define	NR_SUB_PER_PART		11	// 扩展分区最多有11个逻辑分区, mingxuan
#define	NR_SUB_PER_DRIVE	(NR_SUB_PER_PART * NR_PART_PER_DRIVE) //每块硬盘(驱动器)最多有16 * 4 = 64个逻辑分区, mingxuan
#define	NR_PRIM_PER_DRIVE	(NR_PART_PER_DRIVE + 1) // 表示的是hd[0～4]这5个分区，因为有些代码中我们把整块硬盘（hd0）和主分区（hd[1～4]）放在一起看待, mingxuan

/**
 * @def MAX_PRIM
 * Defines the max minor number of the primary partitions.
 * If there are 2 disks, prim_dev ranges in hd[0-9], this macro will
 * equals 9.
 */
// MAX_PRIM定义的是主分区的最大值，比如若有两块硬盘，那第一块硬盘的主分区为hd[1～4]，第二块硬盘的主分区为hd[6～9]
// 所以MAX_PRIM为9，我们定义的hd1a的设备号应大于它，这样通过与MAX_PRIM比较，我们就可以知道一个设备是主分区还是逻辑分区
// mingxuan
#define	MAX_PRIM			(MAX_DRIVES * NR_PRIM_PER_DRIVE - 1)	// MAX_PRIM = 2 * 4 - 1 = 9
#define	MAX_SUBPARTITIONS	(NR_SUB_PER_DRIVE * MAX_DRIVES) 		// MAX_SUBPARTITIONS = 64 * 2 = 128

#define	P_PRIMARY	0
#define	P_EXTENDED	1

// #define ORANGES_PART	0x99	/* Orange'S partition */
#define NO_PART		0x00	/* unused entry */
#define EXT_PART	0x05	/* extended partition */

struct part_info {
	u32	base;		/* # of start sector (NOT byte offset, but SECTOR) */
	u32	size;		/* how many sectors in this partition */
	u32 fs_type;	//added by mingxuan 2020-10-27
};

/* main drive struct, one entry per drive */
struct hd_info
{
	int					open_cnt;
	struct part_info	part[NR_PRIM_PER_DRIVE + NR_SUB_PER_PART];
	// struct part_info	primary[NR_PRIM_PER_DRIVE];	// NR_PRIM_PER_DRIVE = 5
	// struct part_info	logical[NR_SUB_PER_DRIVE];	// NR_SUB_PER_DRIVE = 16 *4 =64
};


/***************/
/* DEFINITIONS */
/***************/
#define	HD_TIMEOUT		10000	/* in millisec */
#define	PARTITION_TABLE_OFFSET	0x1BE
#define ATA_IDENTIFY		0xEC
#define ATA_READ		0x20	//LBA24
#define ATA_READ_EXT			0x24//LBA48,by qianglong 2022.4.25
#define ATA_WRITE		0x30
#define ATA_WRITE_EXT			0x34//LBA48,by qianglong 2022.4.25
/* for DEVICE register. */
#define	MAKE_DEVICE_REG(lba,drv,lba_highest) (((lba) << 6) |		\
					      ((drv) << 4) |		\
					      (lba_highest & 0xF) | 0xA0)

#define HDQUE_MAXSIZE	64

// added by xw, 18/8/26
typedef struct rdwt_info
{
	MESSAGE *msg;
	void *kbuf;
	PROCESS *proc;
	struct rdwt_info *next;
} RWInfo;

typedef struct
{
	RWInfo *front;
	RWInfo *rear;
} HDQueue;

PUBLIC void init_hd();
PUBLIC void hd_open(int device);
PUBLIC void hd_close(int device);
PUBLIC void hd_service();

PUBLIC void hd_rdwt(MESSAGE *p);
PUBLIC void hd_rdwt_sched(MESSAGE *p);
PUBLIC void hd_ioctl(MESSAGE *p);


// add by sundong 2023.6.3 将读写扇区函数的封装由文件系统层放在了驱动层
int rw_sector(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf);
int rw_sector_sched(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf);

//add by sundong 2023.5.26 读取数据块
int rw_blocks(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf);
int rw_blocks_sched(int io_type, int dev, u64 pos, int bytes, int proc_nr, void *buf);

//~xw
//硬盘中数据块的读写
//added by sundong 2023.5.26
#define RD_BLOCK_SCHED(dev,block_nr,fsbuf) rw_blocks_sched(DEV_READ, \
				       dev,				\
				       (u64)(block_nr) * BLOCK_SIZE,		\
				       BLOCK_SIZE, /* read one block */ \
				       proc2pid(p_proc_current),/*current task id*/			\
				       fsbuf);
//added by sundong 2023.5.26
#define WR_BLOCK_SCHED(dev,block_nr,fsbuf) rw_blocks_sched(DEV_WRITE, \
				       dev,				\
				       (u64)(block_nr) * BLOCK_SIZE,		\
				       BLOCK_SIZE, /* write one block */ \
				       proc2pid(p_proc_current),				\
				       fsbuf);
					
#define RD_BLOCK(dev,block_nr,fsbuf) rw_blocks(DEV_READ, \
				       dev,				\
				       (u64)(block_nr) * BLOCK_SIZE,		\
				       BLOCK_SIZE, /* read one block */ \
				       proc2pid(p_proc_current),/*current task id*/			\
				       fsbuf);
//added by sundong 2023.5.26
#define WR_BLOCK(dev,block_nr,fsbuf) rw_blocks(DEV_WRITE, \
				       dev,				\
				       (u64)(block_nr) * BLOCK_SIZE,		\
				       BLOCK_SIZE, /* write one block */ \
				       proc2pid(p_proc_current),				\
				       fsbuf);

#endif /* _ORANGES_HD_H_ */
