//ported by sundong 2023.3.26
#include "disk.h"
#include "loadkernel.h"
#include "x86.h"
#include "fat32.h"
#include "loaderprint.h"
#include "ahci.h"
u32 bootPartStartSector;
bool found_sata_dev = false;
static void ide_waitdisk(void) {
  // wait for disk reaady
  while ((inb(0x1F7) & 0xC0) != DISK_READY_FLAG) /* do nothing */
    ;
}
static bool find_act_part_in_ext(void *buffer,u32 ext_start_sector){
    u32 logical_part_start_sector = ext_start_sector;
    //lprintf("ext start sect %d \n",ext_start_sector);
    readsect(buffer,logical_part_start_sector);
    part_tbl_entry *p_ebr_entry = (buffer+MBR_CODE_LEN);
    //lprintf("ext active flag %d lba start %d\n",p_ebr_entry->status,p_ebr_entry->lba);
    if(p_ebr_entry->status == ACT_PART_FLAG){
      bootPartStartSector = logical_part_start_sector+p_ebr_entry->lba;
      return true;
    }
    p_ebr_entry++;
    if(p_ebr_entry->lba){
      logical_part_start_sector+=p_ebr_entry->lba;
      return find_act_part_in_ext(buffer,logical_part_start_sector);
    }
    return false;

}
void find_act_part(void *dst) {
  u32 offset = 0;  // mbr在0号扇区
  // 在保护模式下读取磁盘内容
/*   ide_waitdisk();

  outb(0x1F2, 1);             // count = 1读写扇区数量
  outb(0x1F3, offset);        // LBA参数的0-7位
  outb(0x1F4, offset >> 8);   // LBA参数的8-15位
  outb(0x1F5, offset >> 16);  // LBA参数的16-23位
  outb(0x1F6, (offset >> 24) |
                  0xE0);  // LBA模式是24-27位为0-3位，第四位0为主盘，1为从盘
  outb(0x1F7,
       CMD_READ);  // cmd 0x20 - read sectors状态和命令寄存器，操作时先给命令再读取

  // wait for disk to be ready
  ide_waitdisk();

  // read a sector
  insl(0x1F0, dst, SECTSIZE / 4);  // 从port0x1F0读取SECTSIZE/4个双字到dst中 */
  readsect(dst,offset);
 
  BYTE *start = (BYTE *)(dst + MBR_CODE_LEN);  // 查找后64个字节的分区信息
  part_tbl_entry *p_mbr_entry = (part_tbl_entry *)start;
  for (int i = 0; i < PART_NUM; i++)  // 只有4个分区，每个分区16个字节
  {
    lprintf("partition_type %d start sector %d style %d \n",p_mbr_entry->status,p_mbr_entry->lba,p_mbr_entry->partition_type);
    if (p_mbr_entry->status == ACT_PART_FLAG)  // 活跃分区
    {
/*       u32 temp = 0;
      start = start + 12;  // 分区的起始扇区号在8字节处，长度为4个字节，从后往前读
      temp = temp + (u32)(*start);
      for (int j = 0; j < 4; j++)  // 读取4个字节
      {
        start = start - 1;
        temp = (temp << 8) + (u8)(*start);
      } */
      bootPartStartSector = p_mbr_entry->lba;  // 给全局变量赋值
      return ;
    }
    //遍历扩展分区
    if(p_mbr_entry->partition_type == EXT_PART_FALG){
        u32 ext_part_start_sect = p_mbr_entry->lba;
        lprintf("ext_part_start_sect %d\n",ext_part_start_sect);
        void *ebr_buff = dst+SECTSIZE;
        if(find_act_part_in_ext(ebr_buff,ext_part_start_sect)){
            return ;
        };
      
    }
    p_mbr_entry++;
    //start = start + PART_TABLE_ENTRY_SIZE;  // 查找下一个分区
  }
}
static int ide_readsect(void *dst,u32 offset){
    // wait for disk to be ready
  ide_waitdisk();
  outb(DISK_SECT_COUNT_PORT, 1);             // count = 1读写扇区数量
  outb(DISK_SECT_LBA_0_7_PORT, offset);        // LBA参数的0-7位
  outb(DISK_SECT_LBA_8_15_PORT, offset >> 8);   // LBA参数的8-15位
  outb(DISK_SECT_LBA_16_23_PORT, offset >> 16);  // LBA参数的16-23位
  outb(DISK_SECT_LBA_24_31_PORT, (offset >> 24) | 0xE0);  // LBA模式是24-27位为0-3位，第四位0为主盘，1为从盘
  outb(DISK_SECT_CMD_PORT,CMD_READ);  // cmd 0x20 - read sectors状态和命令寄存器，操作时先给命令再读取
  // wait for disk to be ready
  ide_waitdisk();
  // read a sector
  insl(DISK_PORT, dst, (SECTSIZE / 4));  // 从port0x1F0读取SECTSIZE/4个双字到dst中 
  return TRUE;
}
static int ide_read(void *dst, u32 offset,int count){
  if (count < 0 || count > 8)
  {
    lprintf("ide_read para err !\n");
    return FALSE;
  }
  int ret = 1;
  for (int i = 0; i < count; i++)
  {
    ret &= ide_readsect(dst+i*SECTSIZE,offset+i);
  }
  return ret;
}
//只读一个sect
int readsect(void *dst, u32 offset) {
  return readsects(dst,offset,1);
}
//一次读多个sector
int readsects(void* dst, u32 offset,u32 count){
  if(found_sata_dev)  return sata_read(0,dst,offset,count);
  else                return ide_read(dst,offset,count);
  //ide_read(dst,offset,count);
}

