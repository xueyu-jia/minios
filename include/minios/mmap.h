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

/*!
 * \brief map pa into kernel space
 *
 * \return mapped va
 * \retval NULL failed to find an available va
 *
 * \note for pa over 1g, map into kernel shared mmap address space
 * \attention you're expected to maintain the user_va field in page mannually when needed
 */
void *kmap(memory_page_t *page);

/*!
 * \brief unmap pa from kernel space if needed
 *
 * \attention you're expected to maintain the user_va field in page mannually when needed
 */
void kunmap(memory_page_t *page);

int kern_mmap(process_t *p_proc, struct file_desc *file, u32 addr, u32 len, u32 prot, u32 flag,
              u32 pgoff);
int kern_munmap(process_t *p_proc, u32 start, u32 len);

int kern_mmap_safe(u32 addr, u32 len, u32 prot, u32 flag, int fd, u32 offset);
