//ported by sundong 2023.3.26
#include "fat32.h"
#include "disk.h"
/*
获取下一个扇区号
扇区号 = 簇号*4/每个扇区的字节数 + （隐藏扇区数 +
保留扇区数）【相当于加上fat起始扇区号】 扇区偏移 = 簇号*4%每个扇区的字节数
*/
static u32 get_next_clus(u32 current_clus) {
    u32 sec = current_clus * 4 / SECTSIZE;  // 需要乘4是因为每个簇号大小为4字节
    u32 off = current_clus * 4 % SECTSIZE;
    if (fat_now_sec != fat_start_sec + sec) {  // 若当前簇号不为FAT32表所在的扇区的簇号则重新读取并更新当前簇号
      readsect((void *)BUF_ADDR, bootPartStartSector+fat_start_sec + sec);
      fat_now_sec = fat_start_sec + sec;
    }
    return *(u32 *)(BUF_ADDR + off);  // 返回下一个簇的簇号
}
//将文件名转为大写 .转为"  "
static void fat32_uppercase_filename(char*src,char*dst){
  char *p = src;
  int index = 0;
  int len = strlen(src)+1;
  while(len){
    if(*p >= 'a'&&*p <= 'z'){
      dst[index++] = *p-32;
    }
    else if(*p=='.'){
      //"."转化为两个空格
      dst[index++]=' ';
      dst[index++]=' ';
    }
    else{
      dst[index++] = *p;
      if(*p == "\0")return;
    }
    p++;
    len--;
  }

}
/*
 * 读入簇号对应的数据区的所有数据
 */
static void * read_cluster(void *dst, u32 current_clus) {
    // 计算簇号对应的数据区的簇号 = （簇号-2）*每个簇的扇区数+数据区起始扇区数
    int data_clus = current_clus - bpb.BPB_RootClus; 
    int clus_start_sect = data_clus * bpb.BPB_SecPerClus + data_start_sec;
    // 根据每个簇包含的扇区数量将数据读入目的地址
    //for (int i = 0; i < bpb.BPB_SecPerClus; i++, dst += SECTSIZE)
      //readsect(dst, bootPartStartSector+clus_start_sect + i); 
    readsects(dst, bootPartStartSector+clus_start_sect, bpb.BPB_SecPerClus);
    return dst+bpb.BPB_SecPerClus*SECTSIZE;
}

void fat32_init(){
    readsect((void *)&bpb,bootPartStartSector);  // 读取FAT32的信息，写入BPB结构体
    // 当扇区字节大小不为512时判断为错误
    if ( bpb.BPB_BytsPerSec != SECTSIZE) goto bad;
    fat_start_sec = bpb.BPB_RsvdSecCnt;  // 计算fat表起始扇区号
    data_start_sec = fat_start_sec + bpb.BPB_FATSz32 * bpb.BPB_NumFATs ;  // 计算data起始扇区号
    return ;
    bad:
    while (1);
}

int fat32_read_file(char *filename,void *dst){
    char filename_upper[strlen(filename)+1];
    fat32_uppercase_filename(filename,filename_upper);
    filename = filename_upper;
    u32 root_clus = bpb.BPB_RootClus;  // 根目录簇号
    while (root_clus < 0x0FFFFFF8) {
      void *read_end = read_cluster((void *)BUF_ADDR, root_clus);  // 将根目录区的所有数据读入缓冲区，返回最后的地址
      // 遍历根目录所有文件，寻找kernel
      for (struct Directory_Entry *p = (void *)BUF_ADDR; (void *)p < read_end;p++) {  // 结构体中存储文件描述信息
        if (strncmp(p->DIR_Name, filename, strlen(filename)) == 0) {  // 比较字符串，判断是否找到kernel.bin
          elf_clus = (u32)p->DIR_FstClusHI << 16 | p->DIR_FstClusLO;  // 记录kernel.bin所在的扇区号
          break;
        }
      }
      if (elf_clus != 0)  // 若搜完所有簇还没找到就是失败
        break;
      root_clus = get_next_clus(root_clus);  // 循环寻找下一个簇
    }

    if (elf_clus == 0) goto bad;

    // 将ELF文件的所有数据从磁盘中读入ELF_ADDR
    while (elf_clus < 0x0FFFFFF8) {
      dst = read_cluster(dst, elf_clus);
      elf_clus = get_next_clus(elf_clus);
    }
    return TRUE;
    bad:
    return FALSE;

	

}
