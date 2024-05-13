#include "const.h"
#include "mempage.h"
#include "buddy.h"
#include "memman.h"
#include "fs.h"
#include "list.h"

// 向mem_pages结构添加新的物理页
// note: 此处不检查是否已经存在页面，caller应保证在调用此函数之前获得互斥锁之后检查find_cache_page
// require: mem_pages.lock
PUBLIC void add_mem_page(mem_pages *page_cache, page *_page) {
	page* next = NULL;
	struct list_node *page_list_head = &page_cache->page_list;
	list_for_each(page_list_head, next, pg_list)
	{
		if(next->pg_off > _page->pg_off)break;
		if(next->pg_off == _page->pg_off) {
			disp_str("error: multi pages to be inserted?");
		}
	}
	list_add_before(&_page->pg_list, (next)?(&next->pg_list):page_list_head);
	_page->pg_cache = page_cache;
}

// 在mem_pages结构中查找对应page offset的物理页
// require: mem_pages.lock
PUBLIC page *find_mem_page(mem_pages *page_cache, u32 pgoff) {
	return list_find_key(&page_cache->page_list, page, pg_list, pg_off, pgoff);
}

PUBLIC void init_mem_page(mem_pages *page_cache, int type) {
    list_init(&page_cache->page_list);
	page_cache->type = type;
}

PUBLIC void writeback_mem_page(mem_pages *page_cache) {
	if(page_cache->page_mapping == NULL) {
		return;
	}
	page *_page = NULL;
	list_for_each(&page_cache->page_list, _page, pg_list)
	{
		if(_page->dirty){
			generic_file_writepage(page_cache->page_mapping, _page);
			_page->dirty = 0;
		}
	}
}

// release page from page cache
// write back if dirty
// require _page->pg_cache lock
PUBLIC int free_mem_page(page *_page) 
{
	struct address_space * mapping = &_page->pg_cache->page_mapping;
	if(_page->dirty && mapping){
		generic_file_writepage(mapping, _page);
		_page->dirty = 0;
	}
	for(int i = 0; i < MAX_BUF_PAGE; i++) {
		if(_page->pg_buffer[i] != NULL){
			kern_kfree(_page->pg_buffer[i]);
		}
	}
	list_remove(&_page->pg_list);
	return free_pages(ubud, _page, 0);
}

// 
// write back dirty pages
// require _page->pg_cache lock


/// @brief 释放page cache中的所有
/// @param page_cache 
/// @return 
PUBLIC void free_mem_pages(mem_pages *page_cache) {
	page * _page = NULL;
	while(!list_empty(&page_cache->page_list)) { // 因为释放操作会改变链表，所以不能使用list_for_each
		_page = list_front(&page_cache->page_list, page, pg_list);
		if(atomic_get(&_page->count) > 0) {
			disp_str("error: free busy page\n");
		}
		free_mem_page(_page);
	}
}