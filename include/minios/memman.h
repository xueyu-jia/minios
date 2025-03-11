#pragma once

#include <minios/mempage.h>
#include <minios/page.h>
#include <minios/proc.h>
#include <minios/layout.h>
#include <klib/size.h>
#include <klib/stddef.h>
#include <klib/sys/types.h>
#include <list.h>

#define FAULT_NOPAGE 1
#define FAULT_WRITE 2

extern memory_page_t* mem_map;
extern size_t nr_mmap_pages;

#define pfn_to_page(pfn) (mem_map + (pfn))
#define page_to_pfn(page) ((page) - mem_map)
#define kpage_lin(page) K_PHY2LIN(pfn_to_phy(page_to_pfn((page))))

#define PHY_NULL (~(phyaddr_t)0)

void memory_init();

phyaddr_t phy_kmalloc(size_t size);
void phy_kfree(phyaddr_t phy_addr);

phyaddr_t phy_malloc_4k();
void phy_free_4k(phyaddr_t phy_addr);

void* kern_kmalloc(size_t size);
void* kern_kzalloc(size_t size);
void kern_kfree(void* ptr);

void* kern_malloc_4k();
void kern_free_4k(void* ptr);
size_t kern_total_mem_size();

void get_page(memory_page_t* page);
int put_page(memory_page_t* page);

memory_page_t* alloc_user_page(u32 pgoff);

void copy_page(memory_page_t* dst, memory_page_t* src);
void zero_page(memory_page_t* page);
void copy_from_page(memory_page_t* page, void* buf, u32 len, u32 offset);
void copy_to_page(memory_page_t* page, const void* buf, u32 len, u32 offset);

void prepare_vma(process_t* p_proc, memmap_t* mmap, struct vmem_area* vma);
struct vmem_area* find_vma(memmap_t* mmap, u32 addr);
void free_vmas(process_t* p_proc, memmap_t* mmap, struct vmem_area* start, struct vmem_area* last);

void memmap_copy(process_t* p_parent, process_t* p_child);
void memmap_clear(process_t* p_proc);

int handle_mm_fault(memmap_t* mmap, u32 vaddr, int flag);
