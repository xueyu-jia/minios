/**
 * fs_const.h
 * This file contains consts and macros associated with filesystem.
 * The code is added by zcr, and the file is added by xw. 18/6/17
 */

// common macros about fs(common) and dev
// hd moved to hd.h
// orange moved to fs.h

#define BLOCK_SIZE		4096  //add by sundong 2023.5.26
#define BLOCK_SIZE_SHIFT   12
#define	MAX_PATH	128

/* major device numbers (corresponding to kernel/global.c::dd_map[]) */
#define	NO_DEV			0
#define	DEV_FLOPPY		1
#define	DEV_CDROM		2
#define	DEV_HD			3
#define	DEV_CHAR_TTY	13
#define	DEV_SCSI		5
#define	DEV_SATA		6

/* make device number from major and minor numbers */
#define	MAJOR_SHIFT		20
#define	MAKE_DEV(a,b)		((a << MAJOR_SHIFT) | b)

/* separate major and minor numbers from device number */
#define	MAJOR(x)		((x >> MAJOR_SHIFT) & 0x0FFF)
#define	MINOR(x)		(x & 0x0FFFFF)


// #define	NR_FILES	64	//moved to proc.h. xw, 18/6/14
//#define	NR_FILE_DESC	64	/* FIXME */	//deleted by mingxuan 2019-5-19
#define	NR_FILE_DESC	128	/* FIXME */	//modified by mingxuan 2019-5-19
#define	NR_INODE	512	/* FIXME */
#define	NR_SUPER_BLOCK	16

/* APIs of file operation */
// octol
#define	O_CREAT		4
#define O_ACC		3
#define O_RDONLY	0
#define O_WRONLY	1
#define	O_RDWR		2
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

// permission
#define I_RWX	7
#define I_R	4
#define I_W	2
#define I_X	1

/* INODE::i_mode (octal, lower 12 bits reserved) */
#define I_TYPE_MASK     0170000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY     0040000
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE	0010000

#define I_MOUNTPOINT	0110000		//add by xiaofeng

#define	is_special(m)	((((m) & I_TYPE_MASK) == I_BLOCK_SPECIAL) ||	\
			 (((m) & I_TYPE_MASK) == I_CHAR_SPECIAL))
#define inode_allow_lseek(type) ((type) != I_CHAR_SPECIAL && (type) != I_NAMED_PIPE)


//deleted by xw, 18/12/27
//#define FSBUF_SIZE	0x100000	//added by xw, 18/6/17
// #define FSBUF_SIZE	0x100000		//added by mingxuan 2019-5-17


#define	IDE_BASE		0
#define	IDE_LIMIT		4
#define	SATA_BASE		4
#define SATA_LIMIT		8
#define	SCSI_BASE		8
#define	SCSI_LIMIT		8
