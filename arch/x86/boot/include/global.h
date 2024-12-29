#pragma once

#include <type.h>

#define KERNEL_FILENAME "kernel.bin"
#define BSS_SECTION_NAME ".bss"

extern u32 fat_start_sec;
extern u32 data_start_sec;
extern u32 elf_off;
extern u32 fat_now_sec;
extern u32 elf_first;
extern struct BPB bpb;
