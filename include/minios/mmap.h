#pragma once

#include <minios/mempage.h>
#include <minios/proc.h>
#include <klib/stdint.h>

struct file_desc;

#define MAP_SHARED 8
#define MAP_PRIVATE 16
#define MAP_FIXED 32

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

void *kmap(memory_page_t *page);
void kunmap(memory_page_t *page);

int kern_mmap(process_t *p_proc, struct file_desc *file, u32 addr, u32 len, u32 prot, u32 flag,
              u32 pgoff);
int kern_munmap(process_t *p_proc, u32 start, u32 len);

int kern_mmap_safe(u32 addr, u32 len, u32 prot, u32 flag, int fd, u32 offset);
