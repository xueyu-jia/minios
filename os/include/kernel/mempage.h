#ifndef MEMPAGE_H
#define MEMPAGE_H

#include <klib/list.h>
#include <klib/spinlock.h>

#define MEMPAGE_AUTO	0
#define MEMPAGE_CACHED	1
#define MEMPAGE_SAVE	2
struct address_space;

typedef struct page page;
// mem pages PUBLIC function
PUBLIC void init_mem_page(struct address_space * mapping, int type);
// **mem_pages lock required**
PUBLIC void add_mem_page(struct address_space *mapping, page *_page);
PUBLIC page *find_mem_page(struct address_space *mapping, u32 pgoff);
PUBLIC void pagecache_writeback(struct address_space *mapping);
PUBLIC int free_mem_page(page *_page);
PUBLIC void free_mem_pages(struct address_space *mapping);
#endif
