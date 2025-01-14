#pragma once

#include <minios/slab.h>
#include <minios/buffer.h>
#include <minios/page.h>
#include <minios/cache.h>
#include <minios/layout.h>
#include <fs/blk_types.h>
#include <klib/atomic.h>
#include <list.h>

#define MEMPAGE_AUTO 0
#define MEMPAGE_CACHED 1
#define MEMPAGE_SAVE 2

#define ALL_PAGES ((u32)(PHY_MEM_SIZE) / PGSIZE)
#define MAX_BUF_PAGE (PGSIZE / SECTOR_SIZE)

typedef struct memory_page {
    atomic_t count;
    u32 inbuddy; // 当前page是否在buddy系统的管理中
    u32 order;   // buddy的order
    struct memory_page *next;
    // slab
    kmem_cache_t *cache; // slab对应的cache
    // page cache
    address_space_t *pg_mapping; // page 所属的page cache结构
    // 对于正在使用的页面，使用pg_list将page加入到对应的page链表
    // 如file address_space中的链表，pcb匿名页面链表等
    struct list_node pg_list;
    int dirty;
    // 对于使用过但为了再次使用的需要在缓存的页面，将pg_lru成员连入page_cache_inactive_lru;
    struct list_node pg_lru;
    buf_head *pg_buffer[MAX_BUF_PAGE];
    u32 pg_off;
} memory_page_t;

void init_mem_page(address_space_t *mapping, int type);

// **mem_pages lock required**
void add_mem_page(address_space_t *mapping, memory_page_t *_page);
memory_page_t *find_mem_page(address_space_t *mapping, u32 pgoff);
void pagecache_writeback(address_space_t *mapping);
int free_mem_page(memory_page_t *_page);
void free_mem_pages(address_space_t *mapping);
