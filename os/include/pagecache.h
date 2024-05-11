#ifndef PAGECACHE_H
#define PAGECACHE_H

#include "list.h"
#include "spinlock.h"

struct address_space;
typedef struct _cached_page{
    struct address_space * page_mapping;
	list_head page_cache_list;
	struct spinlock lock;
}cache_pages;
typedef struct page page;
// page cache PUBLIC function
PUBLIC void init_cache_page(cache_pages *page_cache);
// **cache_pages lock required**
PUBLIC void add_cache_page(cache_pages *page_cache, page *_page);
PUBLIC page *find_cache_page(cache_pages *page_cache, u32 pgoff);
PUBLIC void writeback_cache_page(cache_pages *page_cache);
#endif