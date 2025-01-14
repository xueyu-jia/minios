#pragma once

#include <fs/fs.h>
#include <minios/spinlock.h>
#include <klib/stdint.h>
#include <compiler.h>

struct inode;

struct fat32_bpb {
    char jmp[3];
    char OEM[8];
    u16 BytsPerSec; // 512/1024/.../4096
    u8 SecPerClus;  // 1/2/.../128
    u16 RsvdSecCnt;
    u8 NumFATs;     // 2/1
    u16 RootEntCnt; // 0
    u16 TotSec16;   // 0
    u8 Media;
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
    u8 _reserved[12];
    u8 DrvNum;
    u8 _reserved1;
    u8 BootSig;
    u32 VolId;
    char VolLab[11];
    char FilSysType[8];
} PACKED;

#define FS_INFO_SECTOR_OFFSET 0x1e0

struct fat_fsinfo {
    char reserved[4];
    u32 signature; // 0x61417272
    int cluster_free_count;
    int cluster_next_free;
};

struct fat_dir_entry {
    char name[8], ext[3]; // 短文件名/扩展名
    u8 attr;              //	属性
    u8 res;               //	保留，0
    u8 ctime_10ms;        //	创建时间10ms [0~199]
    u16 ctime;            //	创建时间
    u16 cdate;            //	创建日期
    u16 adate;            //	访问日期
    u16 start_h;          //	起始簇号(高2字节)
    u16 mtime;            //	修改时间
    u16 mdate;            //	修改日期
    u16 start_l;          //	起始簇号(低2字节)
    u32 size;             //	文件大小(字节)
};

#define FAT_ENTRY_SIZE 32
#define FAT_DPS 16 //(SECTOR_SIZE/FAT_ENTRY_SIZE)
#define FAT_DPS_SHIFT 4
#define FAT_ROOT_INO 1
#define FAT_DOT ".          "
#define FAT_DOTDOT "..         "

struct fat_dir_slot {
    u8 order;         //	长目录项序号
    char name0_4[10]; //	长文件名，16位unicode,每个长文件名目录项13个字符
    u8 attr;          //	属性,与短目录项一致
    u8 res;           //	0
    u8 checksum;      //	对应短文件名校验码
    char name5_10[12];
    u16 start; //	长目录项中为0
    char name11_12[4];
};

//	FAT目录项属性
#define ATTR_RO 1
#define ATTR_HIDDEN 2
#define ATTR_SYSTEM 4
#define ATTR_VOL 8
#define ATTR_DIR 16
#define ATTR_ARCH 32
#define ATTR_LNAME (ATTR_RO | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOL)

//	FAT短文件名目录项第一个字节标志/长文件名目录项order标志
#define DIR_LDIR_END 0x40
#define DIR_DELETE 0xE5

struct fat32_sb_info {
    int tot_block; // note: sector here all means 512 bytes sector, not logical
                   // sector in bpb
    int fat_size;  // fat sector num
    u8 fat_num;
    u8 fat_bit;
    u16 fsinfo_block;
    int fat_start_block;
    int dir_start_block;
    int data_start_block;
    int cluster_block;
    int max_cluster; // max valid cluster
    int root_cluster;
    struct fat_fsinfo fsinfo;
};

struct fat_info {
    int cluster_start;
    int length; // 连续簇个数
    struct fat_info* next;
};

struct fat32_inode_info {
    struct fat_info* fat_info;
};

#define FAT_SB(sb) ((struct fat32_sb_info*)((sb)->sb_private))
#define FAT_INODE(inode) ((struct fat32_inode_info*)((inode)->i_private))

#define FAT32_END (0xFFFFFFF)
#define FAT_END FAT32_END

extern struct superblock_operations fat32_sb_ops;
extern struct inode_operations fat32_inode_ops;
extern struct dentry_operations fat32_dentry_ops;
extern struct file_operations fat32_file_ops;

int fat32_sync_inode(struct inode* inode);
