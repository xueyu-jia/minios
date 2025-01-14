#pragma once

#include <minios/mempage.h>
#include <minios/page.h>
#include <minios/proc.h>
#include <minios/layout.h>
#include <klib/size.h>
#include <klib/stddef.h>
#include <klib/sys/types.h>
#include <list.h>

#define WALL SZ_256M
#define KKWALL1 SZ_4M
#define KUWALL1 SZ_8M
#define KKWALL2 SZ_8M
#define KUWALL2 SZ_32M

#define FAULT_NOPAGE 1
#define FAULT_WRITE 2

extern u32 kernel_size;
extern bool big_kernel;
extern memory_page_t mem_map[ALL_PAGES];

#define pfn_to_page(x) (mem_map + (x))
#define page_to_pfn(x) ((x) - mem_map)
#define kpage_lin(page) K_PHY2LIN(pfn_to_phy(page_to_pfn((page))))

void memory_init();

u32 phy_kmalloc(u32 size);
u32 phy_kfree(u32 phy_addr);
u32 kern_kmalloc(u32 size);
u32 kern_kzalloc(u32 size);
u32 kern_kfree(u32 addr);
u32 phy_kmalloc_4k();
u32 phy_kfree_4k(u32 phy_addr);

u32 kern_kmalloc_4k();
u32 kern_kfree_4k(u32 addr);
u32 phy_malloc_4k();
u32 phy_free_4k(u32 phy_addr);

u32 kern_malloc_4k();
void kern_free_4k(void* va);

memory_page_t* alloc_user_page(u32 pgoff);
void get_page(memory_page_t* _page);
int put_page(memory_page_t* _page);
void zero_page(memory_page_t* _page);
void copy_from_page(memory_page_t* _page, void* buf, u32 len, u32 offset);
void copy_to_page(memory_page_t* _page, const void* buf, u32 len, u32 offset);
struct vmem_area* find_vma(memmap_t* mmap, u32 addr);
void memmap_copy(process_t* p_parent, process_t* p_child);
void memmap_clear(process_t* p_proc);
void prepare_vma(process_t* p_proc, memmap_t* mmap, struct vmem_area* vma);
void free_vmas(process_t* p_proc, memmap_t* mmap, struct vmem_area* start, struct vmem_area* last);
int handle_mm_fault(memmap_t* mmap, u32 vaddr, int flag);
u32 kern_total_mem_size();
