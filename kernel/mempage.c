#include <minios/buddy.h>
#include <minios/const.h>
#include <minios/fs.h>
#include <minios/memman.h>
#include <minios/mempage.h>
#include <minios/console.h>
#include <list.h>

/// @brief 向address_space结构添加新的物理页
// note:
// 此处不检查是否已经存在页面，caller应保证在调用此函数之前获得互斥锁之后检查find_cache_page
// require: address_space.lock
void add_mem_page(struct address_space *mapping, memory_page_t *page) {
    memory_page_t *next = NULL;
    struct list_node *page_list_head = &mapping->page_list;
    list_for_each(page_list_head, next, pg_list) {
        if (next->pg_off >= page->pg_off) break;
        // 事实上，此处允许存在多个同一pgoff的页存在（当且仅当OPT_MMU_COW开启,fork之后出现）
        // 跳出条件设为>=保证在插入之后查找时会找到新插入的页
        // if(next->pg_off == page->pg_off) {
        // 	kprintf("error: multi pages to be inserted?");
        // }
    }
    list_add_before(&page->pg_list, (next) ? (&next->pg_list) : page_list_head);
    page->pg_mapping = mapping;
}

/// @brief 在address_space结构中查找对应page offset的物理页
// 返回第一个匹配的物理页
// require: address_space.lock
memory_page_t *find_mem_page(struct address_space *mapping, u32 pgoff) {
    return list_find_key(&mapping->page_list, memory_page_t, pg_list, pg_off, pgoff);
}

void init_mem_page(struct address_space *mapping, int type) {
    list_init(&mapping->page_list);
    mapping->type = type;
}

static void pagecache_writeback_one(memory_page_t *_page) {
    struct address_space *mapping = _page->pg_mapping;
    if (_page->dirty && mapping->host) {
        generic_file_writepage(mapping, _page);
        _page->dirty = 0;
    }
}

/// @brief 写回page cache
/// @param mapping
/// @return
void pagecache_writeback(struct address_space *mapping) {
    if (mapping->host == NULL) { return; }
    memory_page_t *_page = NULL;
    list_for_each(&mapping->page_list, _page, pg_list) {
        pagecache_writeback_one(_page);
    }
}

/// @brief 释放page cache中的一个物理页
/// @param _page
/// @return
/// @details
// release page from mem pages
// write back if dirty
// require _page->pg_mapping lock
int free_mem_page(memory_page_t *_page) {
    if (_page->dirty) pagecache_writeback_one(_page);
    list_remove(&_page->pg_list);
    return free_pages(ubud, _page, 0);
}

/// @brief 释放page cache中的所有物理页
/// @param mapping
/// @return
/// @details
// write back dirty pages if any
// require _page->pg_mapping lock
void free_mem_pages(struct address_space *mapping) {
    memory_page_t *_page = NULL;
    int cnt = 0;
    while (!list_empty(&mapping->page_list)) { // 因为释放操作会改变链表，所以不能使用list_for_each
        _page = list_front(&mapping->page_list, memory_page_t, pg_list);
        if (atomic_get(&_page->count) > 0) { kprintf("error: free busy page\n"); }
        cnt++;
        free_mem_page(_page);
    }
    UNUSED(cnt);
}
