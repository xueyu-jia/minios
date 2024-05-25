#ifndef MEM_MAN_H
#define MEM_MAN_H
#include "type.h"				
#include "const.h"
#include "slab.h"
#include "atomic.h"
#include "spinlock.h"
#include "mempage.h"
#include "buffer.h"
#include "proc.h"

#define FMIBuff		0x007ff000	//loader中getFreeMemInfo返回值存放起始地址(7M1020K)

#define WALL            0x10000000            //256M

#define KKWALL1		0x00400000            //4M
#define KUWALL1		0x00800000            //8M

#define KKWALL2         0x00800000            //8M
#define KUWALL2         0x02000000            //32M

#define TEST		0x11223344

#define PHY_MEM_SIZE    0x04000000            //64M
#define ALL_PAGES       ((PHY_MEM_SIZE)/PAGE_SIZE)
#define MAX_BUF_PAGE	(PAGE_SIZE/SECTOR_SIZE)

#define PROT_NONE   0
#define PROT_READ   1
#define PROT_WRITE  2
#define PROT_EXEC   4
#define MAP_SHARED  8
#define MAP_PRIVATE 16
#define MAP_FIXED   32

#define FAULT_NOPAGE	1
#define FAULT_WRITE		2

typedef struct page
{
	atomic_t count;
	// buddy
    u32 inbuddy;            // 当前page是否在buddy系统的管理中
    u32 order;              // buddy的order
    struct page *next;
	// slab
    kmem_cache_t *cache;    // slab对应的cache
	// page cache
	struct address_space* pg_mapping;  // page 所属的page cache结构
	// 对于正在使用的页面，使用pg_list将page加入到对应的page链表
	// 如file address_space中的链表，pcb匿名页面链表等
	struct list_node pg_list;
	int dirty;
	// 对于使用过但为了再次使用的需要在缓存的页面，将pg_lru成员连入page_cache_inactive_lru;
	struct list_node pg_lru;
	buf_head* pg_buffer[MAX_BUF_PAGE];
	u32 pg_off;
}page;

extern list_head page_inactive_lru;
extern u32 kernel_size;
extern int big_kernel;
extern u32 kernel_code_size;  //为内核代码数据分配的内存大小，     added by wang 2021.8.27
extern u32 test_phy_mem_size; //检测到的物理机的物理内存的大小，    added by wang 2021.8.27
extern page mem_map[ALL_PAGES];
#define phy_to_pfn(x)   ((x) / PAGE_SIZE)
#define pfn_to_phy(x)   ((x) * PAGE_SIZE)
#define pfn_to_page(x)  (mem_map + (x))
#define page_to_pfn(x)  ((x) - mem_map)
#define kpage_lin(_page) K_PHY2LIN(pfn_to_phy(page_to_pfn((_page))))
void memory_init();
PUBLIC u32 phy_kmalloc(u32 size);
PUBLIC u32 phy_kfree(u32 phy_addr);
PUBLIC u32 kern_kmalloc(u32 size);
PUBLIC u32 kern_kfree(u32 addr);
PUBLIC u32 phy_kmalloc_4k();
PUBLIC u32 phy_kfree_4k(u32 phy_addr);
// PUBLIC void phy_mapping_4k(u32 phy_addr);
PUBLIC u32 kern_kmalloc_4k();
PUBLIC u32 kern_kfree_4k(u32 addr);
PUBLIC u32 phy_malloc_4k();
PUBLIC u32 phy_free_4k(u32 phy_addr);
PUBLIC int kern_free_4k(void *AddrLin);
// PUBLIC int kern_mapping_4k(u32 AddrLin, u32 pid, u32 phyAddr, u32 pte_attribute);
// PUBLIC int ker_umalloc_4k(u32 AddrLin, u32 pid, u32 pte_attribute);
// PUBLIC int ker_ufree_4k(u32 pid, u32 AddrLin);
PUBLIC u32 kern_malloc_4k();
PUBLIC page* alloc_user_page(u32 pgoff);
PUBLIC void get_page(page *_page);
PUBLIC int put_page(page *_page);
PUBLIC void zero_page(page *_page);
PUBLIC void copy_from_page(page *_page, void* buf, u32 len, u32 offset);
PUBLIC void copy_to_page(page *_page, const void* buf, u32 len, u32 offset);
struct vmem_area * find_vma(LIN_MEMMAP* mmap, u32 addr);
void memmap_copy(PROCESS* p_parent, PROCESS* p_child);
void memmap_clear(PROCESS* p_proc);
void prepare_vma(PROCESS* p_proc, LIN_MEMMAP* mmap, struct vmem_area *vma);
void free_vmas(PROCESS* p_proc, LIN_MEMMAP* mmap, struct vmem_area *start, struct vmem_area *last);
int handle_mm_fault(LIN_MEMMAP* mmap, u32 vaddr, int flag);
#endif
