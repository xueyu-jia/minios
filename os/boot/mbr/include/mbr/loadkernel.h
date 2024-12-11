/*
 * @Author: Yuanhong.Yu
 * @Date: 2023-01-04 16:42:35
 * @Last Modified by: Yuanhong.Yu
 * @Last Modified time: 2023-01-04 16:44:37
 */
// ported by sundong 2023.3.26
#ifndef _LOADKERNEL_H_
#define _LOADKERNEL_H_
// #include "disk.h"
// #include "string.h"
#include "type.h"
#define KERNEL_FILENAME "kernel.bin"
#define BSS_SECTION_NAME ".bss"
extern u32 fat_start_sec;
extern u32 data_start_sec;
extern u32 elf_off;
extern u32 fat_now_sec;
extern u32 elf_first;
extern struct BPB bpb;
void loader_cstart(u32 MemChkBuf, u32 MCRNumber);
void load_kernel();
#endif