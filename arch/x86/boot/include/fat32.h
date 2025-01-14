#pragma once

#include <type.h>
#include <global.h>

struct BPB {
    u8 BS_jmpBoot[3];    // EB 58 90 跳转指令
    u8 BS_OEMName[8];    // OEM
    u16 BPB_BytsPerSec;  // 每扇区字节数
    u8 BPB_SecPerClus;   // 每簇扇区数。软盘一般每簇一扇区
    u16 BPB_RsvdSecCnt;  // 保留扇区数
    u8 BPB_NumFATs;      // FAT个数
    u16 BPB_RootEntCnt;  // 根目录项数
    u16 BPB_TotSec16;    // 小扇区数，只有FAT12/16使用
    u8 BPB_Media;        // 媒体描述符，
    u16 BPB_FATSz16;     // 每FAT扇区数
    u16 BPB_SecPerTrk;   // 每道扇区数
    u16 BPB_NumHeads;    // 磁头数
    u32 BPB_HiddSec;     // 隐藏扇区数
    u32 BPB_TotSec32;    // 总扇区数
    u32 BPB_FATSz32;     // 每FAT扇区数量，只被FAT32使用
    u16 BPB_ExtFlags;    // 扩展标志，只被FAT32使用
    u16 BPB_FSVer;       // 文件系统版本
    u32 BPB_RootClus;    // 根目录簇号
    u16 BPB_FSInfo;      // 文件系统信息扇区号
    u16 BPB_BkBootSec;   // 备份引导扇区
    u8 BPB_Reserved[12]; // 保留
    u8 BS_DrvNum;        // 物理驱动器号
    u8 BS_Reserved1;     // 保留
    u8 BS_BootSig;       // 扩展引导标签
    u32 BS_VolID;        // 分区序号
    u8 BS_VolLabp[11];   // 卷标
    u8 BS_FilSysType[8]; // FAT32文件中一般取"FAT32"
    u8 zero[420];        // 引导代码
    u16 Signature_word;  // 结束 0x55AA
} __attribute__((packed));

struct Directory_Entry {
    char DIR_Name[11];
    u8 DIR_Attr;
    u8 DIR_NTRes;
    u8 DIR_CrtTimeTenth;
    u16 DIR_CrtTime;
    u16 DIR_CrtDate;
    u16 DIR_LstAccDate;
    u16 DIR_FstClusHI;
    u16 DIR_WrtTime;
    u16 DIR_WrtDate;
    u16 DIR_FstClusLO;
    u32 DIR_FileSize;
} __attribute__((packed));

extern struct BPB bpb;
extern u32 fat_start_sec;
extern u32 data_start_sec;
extern u32 elf_off;
extern u32 fat_now_sec;
extern u32 elf_first;

/* u32 get_next_clus_number(u32 current_clus);
void *read_cluster(void *dst, u32 current_clus); */
extern void fat32_init();
// extern int  fat32_read_file(char *filename,void *dst);
extern int fat32_open_file(char *filename);
extern int fat32_read(u32 offset, u32 lenth, void *buf);
