#ifndef MMAP_H
#define MMAP_H
#include "type.h"
#include "const.h"
#include "proc.h"
#include "memman.h"

PUBLIC void * kmap(page *_page);
PUBLIC void kunmap(page *_page);
struct file_desc; //forward declare
int kern_mmap(PROCESS* p_proc, struct file_desc *file, u32 addr, u32 len, 
    u32 prot, u32 flag, u32 pgoff);
int kern_munmap(PROCESS* p_proc, u32 start, u32 len);
int do_mmap(u32 addr, u32 len, u32 prot, u32 flag, int fd, u32 offset);
int do_munmap(u32 start, u32 len);

#endif