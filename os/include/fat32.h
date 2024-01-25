#ifndef FAT32_H
#define FAT32_H

#include "type.h"

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
#define FAT_FLAG_MIRROR 0x80
#define FAT_FLAG_ACTIVE_MASK 0xF

#define FS_INFO_SECTOR_OFFSET 0x1e0
struct fat32_fsinfo{
	char reserved[4];
	u32 signature; //0x61417272
	int cluster_free_count;
	int cluster_next_free;
};

struct fat32_sb_info{
	int tot_sector; // note: sector here all means 512 bytes sector, not logical sector in bpb
	int fat_size; // fat sector num
	u8 fat_num;
	u8 fat_bit;
	u16 fsinfo_sector;
	// int fat_active; // -1 means all fat mirrored, or the only active fat num
	int fat_start_sector;
	int dir_start_sector;
	int data_start_sector;
	int cluster_sector;
	int max_cluster; // max valid cluster
	struct fat32_fsinfo fsinfo;
};

#define FAT_SB(sb) (&((sb)->fat32_sb))
#define FAT32_END  (0xFFFFFFF)
#define FAT_END   FAT32_END
struct fat_ent{
	int cluster_start;
	int length; // 连续簇个数
	struct fat_ent* next;
};

struct fat32_inode_info{
	struct fat_ent* fat_info;
};


extern struct superblock_operations fat32_sb_ops;
extern struct inode_operations fat32_inode_ops;
extern struct dentry_operations fat32_dentry_ops;
extern struct file_operations fat32_file_ops;

#endif