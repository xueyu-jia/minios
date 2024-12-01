#ifndef MMAP_H
#define MMAP_H
#include <kernel/const.h>
#include <kernel/memman.h>
#include <kernel/proc.h>
#include <kernel/type.h>

PUBLIC void *kmap(page *_page);
PUBLIC void kunmap(page *_page);
struct file_desc;  // forward declare
PUBLIC int kern_mmap(PROCESS *p_proc, struct file_desc *file, u32 addr, u32 len,
                     u32 prot, u32 flag, u32 pgoff);
PUBLIC int kern_munmap(PROCESS *p_proc, u32 start, u32 len);
PUBLIC int do_mmap(u32 addr, u32 len, u32 prot, u32 flag, int fd, u32 offset);
PUBLIC int do_munmap(u32 start, u32 len);

#endif
