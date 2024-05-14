#ifndef MEMPAGE_H
#define MEMPAGE_H

#include "list.h"
#include "spinlock.h"

#define MEMPAGE_AUTO	0
#define MEMPAGE_CACHED	1
#define MEMPAGE_SAVE	2
struct address_space;

typedef struct _mem_pages{
	int type;
    struct address_space * page_mapping;
	list_head page_list;
	struct spinlock lock;
}mem_pages;
typedef struct page page;
// mem pages PUBLIC function
PUBLIC void init_mem_page(mem_pages *mem_page, int type);
// **mem_pages lock required**
PUBLIC void add_mem_page(mem_pages *mem_page, page *_page);
PUBLIC page *find_mem_page(mem_pages *mem_page, u32 pgoff);
PUBLIC void writeback_mem_page(mem_pages *mem_page);
PUBLIC int free_mem_page(page *_page);
PUBLIC void free_mem_pages(mem_pages *mem_page);
#endif