/*
 * @Author: Yuanhong.Yu
 * @Date: 2023-01-04 16:42:52
 * @Last Modified by:   Yuanhong.Yu
 * @Last Modified time: 2023-01-04 16:42:52
 */
// ported by sundong 2023.3.26
#ifndef _DISK_H_
#define _DISK_H_
#include "type.h"

#define SECTSIZE 512
// 用来读取ELF程序段头的缓冲区
#define ELF_ADDR 0x7e00
#define ELF_BUF_LEN 0x400
#define BUF_ADDR (ELF_ADDR + ELF_BUF_LEN)
#define ACT_PART_FLAG 0x80
#define DISK_READY_FLAG 0x40

#define IDE_DEV_BASE_PORT 0x1F0
#define DISK_PORT IDE_DEV_BASE_PORT
#define DISK_SECT_COUNT_PORT DISK_PORT + 2
#define DISK_SECT_LBA_0_7_PORT DISK_PORT + 3
#define DISK_SECT_LBA_8_15_PORT DISK_PORT + 4
#define DISK_SECT_LBA_16_23_PORT DISK_PORT + 5
#define DISK_SECT_LBA_24_31_PORT DISK_PORT + 6
#define DISK_SECT_CMD_PORT DISK_PORT + 7

#define CMD_READ 0x20

#define MBR_CODE_LEN 0x1be

#define PART_TABLE_ENTRY_SIZE 16
#define PART_NUM 4
#define EXT_PART_FALG 0x05

extern u32 bootPartStartSector;
extern bool found_sata_dev;

//读一个扇区
int readsect(void* dst, u32 offset);
void find_act_part(void* dst);
//读多个扇区
int readsects(void* dst, u32 offset, u32 count);
typedef struct {
  u8 status;
  u8 first_chs[3];
  u8 partition_type;
  u8 last_chs[3];
  u32 lba;
  u32 sector_count;
} part_tbl_entry;
#endif