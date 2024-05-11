#include "const.h"
#include "pagecache.h"
#include "memman.h"
#include "fs.h"
#include "list.h"

// 向cache_pages结构添加新的物理页
// note: 此处不检查是否已经存在页面，caller应保证在调用此函数之前获得互斥锁之后检查find_cache_page
// require: cache_pages.lock
PUBLIC void add_cache_page(cache_pages *page_cache, page *_page) {
	page* next = NULL;
	struct list_node *page_list_head = &page_cache->page_cache_list;
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

// 在cache_pages结构中查找对应page offset的物理页
// require: cache_pages.lock
PUBLIC page *find_cache_page(cache_pages *page_cache, u32 pgoff) {
	return list_find_key(&page_cache->page_cache_list, page, pg_list, pg_off, pgoff);
}

PUBLIC void init_cache_page(cache_pages *page_cache) {
    list_init(&page_cache->page_cache_list);
}

PUBLIC void writeback_cache_page(cache_pages *page_cache) {
	if(page_cache->page_mapping == NULL) {
		return;
	}
	page *_page = NULL;
	list_for_each(&page_cache->page_cache_list, _page, pg_list)
	{
		if(_page->dirty){
			generic_file_writepage(page_cache->page_mapping, _page);
			_page->dirty = 0;
		}
	}
}