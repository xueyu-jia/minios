/*************************************************************
*     buddy.h     add by wang   2021.3.3
**************************************************************/
#ifndef KERNEL_BUDDY_H
#define KERNEL_BUDDY_H

#include "type.h"
#include "const.h"
#include "slab.h"
#include "atomic.h"


#define FMIBuff		0x007ff000  //必须于load.inc保持一致, mingxuan 2021-8-25

#define MAX_ORDER 11        //页块链表数目

#define WALL            0x10000000            //256M

#define KKWALL1		0x00400000            //4M
#define KUWALL1		0x00800000            //8M

#define KKWALL2         0x00800000            //8M
#define KUWALL2         0x02000000            //32M

#define PHY_MEM_SIZE    0x04000000            //64M
#define PAGE_SIZE       num_4K
#define ALL_PAGES       ((PHY_MEM_SIZE)/PAGE_SIZE)

typedef struct page
{
    u32 order;              // buddy的order
    u32 inbuddy;            // 当前page是否在buddy系统的管理中
    kmem_cache_t *cache;    // slab对应的cache
    struct page *next;
	atomic_t count;
}page;

struct free_area
{
    page *free_list;  //空闲页块线性表          edit by wang 2021.3.9
    u32  free_count;     //空闲页块数目
};

typedef struct buddy
{
    struct free_area free_area[MAX_ORDER]; //伙伴系统数组，每一个数组元素代表一种内存页块
    u32 total_mem_size;                    //added by wang 2021.8.26
    u32 current_mem_size;

}buddy;

page* alloc_pages(buddy *bud, u32 order);   //分配2^order页的页块
int free_pages(buddy *bud, page *page, u32 order);

extern struct page mem_map[ALL_PAGES];
#define phy_to_pfn(x)   ((x) / PAGE_SIZE)
#define pfn_to_phy(x)   ((x) * PAGE_SIZE)
#define pfn_to_page(x)  (mem_map + (x))
#define page_to_pfn(x)  ((x) - mem_map)

extern buddy *kbud;
extern buddy *ubud;
extern u32 block_size[11];

extern u32 kernel_size;
extern int big_kernel;
extern u32 kernel_code_size;  //为内核代码数据分配的内存大小，     added by wang 2021.8.27
extern u32 test_phy_mem_size; //检测到的物理机的物理内存的大小，    added by wang 2021.8.27

void buddy_init(buddy *bud, u32 bgn_addr, u32 end_addr);
int block_init(u32 bgn_addr, u32 end_addr, buddy *bud);
#endif
