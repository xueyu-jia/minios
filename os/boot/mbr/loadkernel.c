// ported by sundong 2023.3.26
#include <mbr/ahci.h>
#include <mbr/disk.h>
#include <mbr/elf.h>
#include <mbr/fs.h>
#include <mbr/loaderprint.h>
#include <mbr/loadkernel.h>
#include <mbr/paging.h>
#include <mbr/string.h>
void loader_cstart(u32 MemChkBuf, u32 MCRNumber) {
    clear_screen();
    lprintf("MemChkBuf: %d MCRNumber %d\n", MemChkBuf, MCRNumber);
    // 启动分页
    paging(MemChkBuf, MCRNumber);
    // 初始化AHCI
    ahci_sata_init();
    // 加载 kernel 并执行 kernel

    load_kernel();
}

#if 0
static bool mem_check_IsZero(int addr, int size) {
  char *p = (char *)addr;
  for (int i = 0; i < size; i++) {
    if (*p != 0) {
      lprintf("check_zero err! addr %d \n", p);
      return FALSE;
    }
    ++p;
  }
  return TRUE;
}
#endif

/*
 * 加载kernel.bin的elf文件并跳过去。
 */
void load_kernel() {
    clear_screen();
    lprintf("----start loading kernel elf----\n");
    find_act_part((void *)BUF_ADDR);
    if (!init_fs()) {
        lprintf("fs type not support !\n");
        goto bad;
    }
    // int ret = read_file(KERNEL_FILENAME,(void *)ELF_ADDR);
    open_file(KERNEL_FILENAME);

    struct Elfdr eh;
    struct Proghdr *ph = (struct Proghdr *)ELF_ADDR;
    struct Secthdr sh;
    // 读elf文件头
    int ret = read(0, sizeof(struct Elfdr), (void *)&eh);
    if (ret != TRUE) goto bad;

    // 读program头
    if (ELF_BUF_LEN < eh.e_phentsize * eh.e_phnum) {
        lprintf("load kernel: ELF Buffer overflow\n");
        goto bad;
    }
    ret &= read(eh.e_phoff, eh.e_phentsize * eh.e_phnum, (void *)ph);

    // 加载program段
    for (int i = 0; i < eh.e_phnum; i++) {
        // ret &= read(eh.e_phoff+eh.e_phentsize*i, eh.e_phentsize, (void
        // *)&ph); print_elf(&ph);
        if (ph[i].p_type != PT_LOAD) continue;
        // 将program写入内存
        ret &= read(ph[i].p_offset, ph[i].p_filesz, (void *)ph[i].p_va);
        memset((void *)ph[i].p_va + ph[i].p_filesz, 0,
               ph[i].p_memsz - ph[i].p_filesz); // 将不需要的内存置为0
    }
    if (ret != TRUE) goto bad;

    lprintf("----finish loading kernel elf----\n");
    lprintf("%d", eh.e_entry);
    ((void (*)(void))(eh.e_entry))();
    while (1);
bad:
    lprintf("----fail to kernel elf----");
    while (1); // do nothing
}
