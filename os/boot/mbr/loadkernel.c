//ported by sundong 2023.3.26
#include "loadkernel.h"
#include "loaderprint.h"
#include "elf.h"
#include "fat32.h"
#include "disk.h"
#include "string.h"
#include "paging.h"
#include "ahci.h"
#include "orangefs.h"
#include "fs.h"
void loader_cstart(u32 MemChkBuf,u32 MCRNumber){
    clear_screen();
    lprintf("MemChkBuf: %d MCRNumber %d \n",MemChkBuf,MCRNumber);
    //启动分页
    paging(MemChkBuf,MCRNumber);
    //初始化AHCI
    AHCI_init();
    //加载kernel并执行kernel
    load_kernel();
}
static bool mem_check_IsZero(int addr,int size){
    char *p = (char *)addr;
    for (int i = 0; i < size; i++)
    {
        if(*p!=0){
            lprintf("check_zero err! addr %d \n",p);
            return FALSE;
        }
        ++p;
    }
    return TRUE;
}
/*
 * 加载kernel.bin的elf文件并跳过去。
 */
void load_kernel() {
    clear_screen();
    lprintf("----start loading kernel elf----\n");
    find_act_part((void *)BUF_ADDR);
    if(!init_fs()){
        lprintf("fs type not support !\n");
        goto bad;
    }
    int ret = read_file(KERNEL_FILENAME,(void *)ELF_ADDR);
    //int ret = fat32_read_file(KERNEL_FILENAME,(void *)ELF_ADDR);
    //判断文件是否读取成功
    if(ret != TRUE)goto bad;
    struct Elf *eh = (struct Elf *)ELF_ADDR;
    struct Proghdr *ph = (struct Proghdr *)((void *)eh + eh->e_phoff);
    for (int i = 0; i < eh->e_phnum;i++, ph++) {  // 遍历所有程序段，加载可加载的程序段
        print_elf(ph);
        if (ph->p_type != PT_LOAD) continue;
        // 一个字节一个字节的把程序段加载到指定位置
        memcpy((void *)ph->p_va, (const void *)eh + ph->p_offset, ph->p_filesz);
        memset((void *)ph->p_va + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);  // 将不需要的内存置为0
    }
    //开始清空bss section
    struct Secthdr *sh = (struct Secthdr *)((void *)eh+eh->e_shoff);
    //获取各个section的命名表
    char * sh_name_tbl = (char*)((void*)eh+sh[eh->e_shstrndx].sh_offset);
    //比较各个section header的名称，是否与BSS_SECTION_NAME相等
    //若相等则将该section置为0
    for (int i = 0; i < eh->e_shnum; i++)
    {
        if(strcmp(sh_name_tbl+sh[i].sh_name,BSS_SECTION_NAME)==0){
            memset(sh[i].sh_addr,0,sh[i].sh_size);
            //检查是否已经设置为0
            if(mem_check_IsZero(sh[i].sh_addr,sh[i].sh_size)){
                break;
            }else{
                goto bad;
            }


        }
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