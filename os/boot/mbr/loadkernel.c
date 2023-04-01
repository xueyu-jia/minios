//ported by sundong 2023.3.26
#include "loadkernel.h"
#include "loaderprint.h"
#include "elf.h"
#include "fat32.h"
#include "disk.h"
#include "string.h"
#include "paging.h"
#include "ahci.h"
void loader_cstart(u32 MemChkBuf,u32 MCRNumber){
  clear_screen();
  //启动分页
  paging(MemChkBuf,MCRNumber);
  //初始化AHCI
  AHCI_init();
  //加载kernel并执行kernel
  load_kernel();
}
/*
 * 初始化函数，加载kernel.bin的elf文件并跳过去。
 */
void load_kernel() {
  //clear_screen();
  lprintf("----start loading kernel elf----\n");
  find_act_part((void *)BUF_ADDR);
  fat32_init();
  int ret = fat32_read_elf_file(KERNEL_FILENAME,(void *)ELF_ADDR);
  //判断文件是否读取成功
  if(ret != TRUE)goto bad;
  struct Elf *eh = (void *)ELF_ADDR;
  struct Proghdr *ph = (void *)eh + eh->e_phoff;
  for (int i = 0; i < eh->e_phnum;i++, ph++) {  // 遍历所有程序段，加载可加载的程序段
    print_elf(ph);
    if (ph->p_type != PT_LOAD) continue;
    // 一个字节一个字节的把程序段加载到指定位置
    memcpy((void *)ph->p_va, (const void *)eh + ph->p_offset, ph->p_filesz);
    memset((void *)ph->p_va + ph->p_filesz, 0,
           ph->p_memsz - ph->p_filesz);  // 将不需要的内存置为0
  }
  lprintf("----finish loading kernel elf----\n");
  lprintf("%d", eh->e_entry);
  ((void (*)(void))(eh->e_entry))();
test:
  while (1)
    ;
bad:
  lprintf("----fail to kernel elf----");
  while (1)
    ;  // do nothing
}