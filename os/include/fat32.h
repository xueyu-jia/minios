#ifndef FAT32_H
#define FAT32_H

#include "type.h"
#include "spinlock.h"

struct __attribute__((packed)) fat32_bpb{
	char jmp[3];
	char OEM[8];
	u16 BytsPerSec;// 512/1024/.../4096
	u8  SecPerClus;// 1/2/.../128
	u16 RsvdSecCnt;
	u8  NumFATs;   // 2/1
	u16 RootEntCnt;// 0
	u16 TotSec16;  // 0
	u8  Media;
	u16 FATSz16;   
	u16 SecPerTrk;
	u16 NumHeads;
	u32 HiddSec;
	u32 TotSec32;
	// fat32 extend bpb
	u32 FATSz32;
	u16 ExtFlags; 
	// bit 7: 0-mirrored FAT;bits 0-3: if mirror disabled,active FAT number
	u16 FSVer;    
	u32 RootClus;
	u16 FSInfo;
	u16 BkBootSec;
	u8  _reserved[12];
	u8  DrvNum;
	u8  _reserved1;
	u8  BootSig;
	u32 VolId;
	char VolLab[11];
	char FilSysType[8];
};

#define FS_INFO_SECTOR_OFFSET 0x1e0
struct fat_fsinfo{
	char reserved[4];
	u32 signature; //0x61417272
	int cluster_free_count;
	int cluster_next_free;
};

struct fat_dir_entry{
	char name[8], ext[3];
	u8 attr;
	u8 res; // 0
	u8 ctime_10ms;
	u16 ctime;
	u16 cdate;
	u16 adate;
	u16 start_h;
	u16 mtime;
	u16 mdate;
	u16 start_l;
	u32 size;
};
#define FAT_ENTRY_SIZE 32
#define FAT_DPS		16 //(SECTOR_SIZE/FAT_ENTRY_SIZE)
#define FAT_DPS_SHIFT 4
#define FAT_ROOT_INO	1
#define FAT_DOT	".          "

struct fat_dir_slot{
	u8 order;
	char name0_4[10];
	u8 attr;
	u8 res; // 0
	u8 checksum;
	char name5_10[12];
	u16 start; // 0 in ldir
	char name11_12[4];
};

#define ATTR_RO		1
#define ATTR_HIDDEN 2
#define ATTR_SYSTEM 4
#define ATTR_VOL	8
#define ATTR_DIR	16
#define ATTR_ARCH	32
#define ATTR_LNAME (ATTR_RO|ATTR_HIDDEN|ATTR_SYSTEM|ATTR_VOL)
#define DIR_LDIR_END 0x40
#define DIR_DELETE	0xE5

struct fat32_sb_info{
	int tot_sector; // note: sector here all means 512 bytes sector, not logical sector in bpb
	int fat_size; // fat sector num
	u8 fat_num;
	u8 fat_bit;
	u16 fsinfo_sector;
	int fat_start_sector;
	int dir_start_sector;
	int data_start_sector;
	int cluster_sector;
	int max_cluster; // max valid cluster
	int root_cluster;
	struct fat_fsinfo fsinfo;
	SPIN_LOCK lock;
};

#define FAT_SB(sb) (&((sb)->fat32_sb))
#define FAT32_END  (0xFFFFFFF)
#define FAT_END   FAT32_END
struct fat_info{
	int cluster_start;
	int length; // 连续簇个数
	struct fat_info* next;
};

struct fat32_inode_info{
	struct fat_info* fat_info;
};


extern struct superblock_operations fat32_sb_ops;
extern struct inode_operations fat32_inode_ops;
extern struct dentry_operations fat32_dentry_ops;
extern struct file_operations fat32_file_ops;

#endif