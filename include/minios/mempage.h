#pragma once

#include <list.h>
#include <klib/spinlock.h>

#define MEMPAGE_AUTO 0
#define MEMPAGE_CACHED 1
#define MEMPAGE_SAVE 2

struct address_space;
typedef struct memory_page memory_page_t;

void init_mem_page(struct address_space *mapping, int type);

// **mem_pages lock required**
void add_mem_page(struct address_space *mapping, memory_page_t *_page);
memory_page_t *find_mem_page(struct address_space *mapping, u32 pgoff);
void pagecache_writeback(struct address_space *mapping);
int free_mem_page(memory_page_t *_page);
void free_mem_pages(struct address_space *mapping);
