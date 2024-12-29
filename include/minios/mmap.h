#ifndef MMAP_H
#define MMAP_H
#include <minios/const.h>
#include <minios/memman.h>
#include <minios/proc.h>
#include <minios/type.h>

void *kmap(memory_page_t *_page);
void kunmap(memory_page_t *_page);
struct file_desc; // forward declare
int kern_mmap(PROCESS *p_proc, struct file_desc *file, u32 addr, u32 len, u32 prot, u32 flag,
              u32 pgoff);
int kern_munmap(PROCESS *p_proc, u32 start, u32 len);
int do_mmap(u32 addr, u32 len, u32 prot, u32 flag, int fd, u32 offset);
int do_munmap(u32 start, u32 len);

#endif
