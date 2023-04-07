//ported by sundong 2023.3.26
#include "disk.h"
#include "loadkernel.h"
#include "x86.h"
#include "fat32.h"
#include "loaderprint.h"
#include "ahci.h"
u32 bootPartStartSector;
bool found_sata_dev = false;
void waitdisk(void) {
  // wait for disk reaady
  while ((inb(0x1F7) & 0xC0) != DISK_READY_FLAG) /* do nothing */
    ;
}
void find_act_part(void *dst) {
  u32 offset = 0;  // mbr在0号扇区
  // 在保护模式下读取磁盘内容
/*   waitdisk();

  outb(0x1F2, 1);             // count = 1读写扇区数量
  outb(0x1F3, offset);        // LBA参数的0-7位
  outb(0x1F4, offset >> 8);   // LBA参数的8-15位
  outb(0x1F5, offset >> 16);  // LBA参数的16-23位
  outb(0x1F6, (offset >> 24) |
                  0xE0);  // LBA模式是24-27位为0-3位，第四位0为主盘，1为从盘
  outb(0x1F7,
       CMD_READ);  // cmd 0x20 - read sectors状态和命令寄存器，操作时先给命令再读取

  // wait for disk to be ready
  waitdisk();

  // read a sector
  insl(0x1F0, dst, SECTSIZE / 4);  // 从port0x1F0读取SECTSIZE/4个双字到dst中 */
  readsect(dst,offset);
  BYTE partition_type;             // 分区类型
  BYTE *start = (BYTE *)(dst + MBR_CODE_LEN);  // 查找后64个字节的分区信息
  for (int i = 0; i < PART_NUM; i++)  // 只有4个分区，每个分区16个字节
  {
    partition_type = (*start);
    if (partition_type == ACT_PART_FLAG)  // 活跃分区
    {
      u32 temp = 0;
      start = start + 12;  // 分区的起始扇区号在8字节处，长度为4个字节，从后往前读
      temp = temp + (u32)(*start);
      for (int j = 0; j < 4; j++)  // 读取4个字节
      {
        start = start - 1;
        temp = (temp << 8) + (u8)(*start);
      }
      bootPartStartSector = temp;  // 给全局变量赋值
      return;
    }
    start = start + PART_TABLE_ENTRY_SIZE;  // 查找下一个分区
  }
}
static void ide_readsect(void *dst,u32 offset){
    // wait for disk to be ready
  waitdisk();
  outb(DISK_SECT_COUNT_PORT, 1);             // count = 1读写扇区数量
  outb(DISK_SECT_LBA_0_7_PORT, offset);        // LBA参数的0-7位
  outb(DISK_SECT_LBA_8_15_PORT, offset >> 8);   // LBA参数的8-15位
  outb(DISK_SECT_LBA_16_23_PORT, offset >> 16);  // LBA参数的16-23位
  outb(DISK_SECT_LBA_24_31_PORT, (offset >> 24) | 0xE0);  // LBA模式是24-27位为0-3位，第四位0为主盘，1为从盘
  outb(DISK_SECT_CMD_PORT,CMD_READ);  // cmd 0x20 - read sectors状态和命令寄存器，操作时先给命令再读取
  // wait for disk to be ready
  waitdisk();
  // read a sector
  insl(DISK_PORT, dst, (SECTSIZE / 4));  // 从port0x1F0读取SECTSIZE/4个双字到dst中 
}
static void ide_read(void *dst, u32 offset,int count){
  if (count < 0 || count > 8)
  {
    lprintf("ide_read para err !\n");
    return ;
  }
  for (int i = 0; i < count; i++)
  {
    ide_readsect(dst+i*SECTSIZE,offset+i);
  }
}
//只读一个sect
void readsect(void *dst, u32 offset) {
  readsects(dst,offset,1);
}
//一次读多个sector
void readsects(void* dst, u32 offset,u32 count){
  if(found_sata_dev)sata_read(0,dst,offset,count);
  else ide_read(dst,offset,count);
  //ide_read(dst,offset,count);
}

