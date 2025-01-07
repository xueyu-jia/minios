#pragma once

#include <minios/atomic.h>
#include <minios/buffer.h>
#include <minios/const.h>
#include <minios/mempage.h>
#include <minios/layout.h>
#include <minios/proc.h>
#include <minios/slab.h>
#include <minios/type.h>
#include <klib/size.h>
#include <klib/spinlock.h>

#define WALL SZ_256M
#define KKWALL1 SZ_4M
#define KUWALL1 SZ_8M
#define KKWALL2 SZ_8M
#define KUWALL2 SZ_32M

#define ALL_PAGES ((u32)(PHY_MEM_SIZE) / PAGE_SIZE)
#define MAX_BUF_PAGE (PAGE_SIZE / SECTOR_SIZE)

#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define MAP_SHARED 8
#define MAP_PRIVATE 16
#define MAP_FIXED 32

#define FAULT_NOPAGE 1
#define FAULT_WRITE 2

typedef struct free_memory_info {
    size_t count;
    struct {
        phyaddr_t base;
        phyaddr_t limit;
    } page_ranges[0];
} free_memory_info_t;

#define __fmi_ptr() ((free_memory_info_t*)u2ptr(0x007ff000))

typedef struct memory_page {
    atomic_t count;
    u32 inbuddy; // 当前page是否在buddy系统的管理中
    u32 order;   // buddy的order
    struct memory_page* next;
    // slab
    kmem_cache_t* cache; // slab对应的cache
    // page cache
    struct address_space* pg_mapping; // page 所属的page cache结构
    // 对于正在使用的页面，使用pg_list将page加入到对应的page链表
    // 如file address_space中的链表，pcb匿名页面链表等
    struct list_node pg_list;
    int dirty;
    // 对于使用过但为了再次使用的需要在缓存的页面，将pg_lru成员连入page_cache_inactive_lru;
    struct list_node pg_lru;
    buf_head* pg_buffer[MAX_BUF_PAGE];
    u32 pg_off;
} memory_page_t;

extern u32 kernel_size;
extern bool big_kernel;
extern memory_page_t mem_map[ALL_PAGES];

#define phy_to_pfn(x) ((x) / PAGE_SIZE)
#define pfn_to_phy(x) ((x) * PAGE_SIZE)
#define pfn_to_page(x) (mem_map + (x))
#define page_to_pfn(x) ((x) - mem_map)
#define kpage_lin(_page) K_PHY2LIN(pfn_to_phy(page_to_pfn((_page))))

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
void kern_free_4k(void* AddrLin);

u32 kern_malloc_4k();
memory_page_t* alloc_user_page(u32 pgoff);
void get_page(memory_page_t* _page);
int put_page(memory_page_t* _page);
void zero_page(memory_page_t* _page);
void copy_from_page(memory_page_t* _page, void* buf, u32 len, u32 offset);
void copy_to_page(memory_page_t* _page, const void* buf, u32 len, u32 offset);
struct vmem_area* find_vma(LIN_MEMMAP* mmap, u32 addr);
void memmap_copy(PROCESS* p_parent, PROCESS* p_child);
void memmap_clear(PROCESS* p_proc);
void prepare_vma(PROCESS* p_proc, LIN_MEMMAP* mmap, struct vmem_area* vma);
void free_vmas(PROCESS* p_proc, LIN_MEMMAP* mmap, struct vmem_area* start, struct vmem_area* last);
int handle_mm_fault(LIN_MEMMAP* mmap, u32 vaddr, int flag);
u32 kern_total_mem_size();
