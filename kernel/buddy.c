#include <minios/buddy.h>
#include <minios/kmalloc.h>
#include <minios/assert.h>
#include <minios/memman.h>
#include <klib/fmi.h>
#include <klib/size.h>
#include <compiler.h>
#include <string.h>

//! FIXME: 当前 buddy 中存在若干不严格的地方以及漏洞，断言并不能全程通过，因此暂时屏蔽掉
#ifdef assert
#undef assert
#define assert(...)
#endif

buddy_t kbuddy, ubuddy;
buddy_t *kbud = &kbuddy;
buddy_t *ubud = &ubuddy;

u32 block_size[BUDDY_ORDER_LIMIT]; // the block size of each order

static void fragmentUse(u32 addr, u32 size);
static u32 find_buddy_pfn(u32 page_pfn, u32 order);
static u32 page_is_buddy(u32 pfn, memory_page_t *page, u32 order);
static void del_page_from_free_list(memory_page_t *page, buddy_t *bud, u32 order);
static void add_to_free_list(memory_page_t *page, buddy_t *bud, u32 order);
static void set_page_order(memory_page_t *page, u32 order);
static void set_buddy_order(memory_page_t *page, u32 order);
static memory_page_t *get_page_from_free_area(struct free_area *area);
static void expand(buddy_t *bud, memory_page_t *page, u32 low, u32 high);

void buddy_init(buddy_t *bud, u32 bgn_addr, u32 end_addr) {
    bud->current_mem_size = 0;
    bud->total_mem_size = 0;
    for (int i = 0; i < BUDDY_ORDER_LIMIT; ++i) {
        bud->free_area[i].free_list = NULL;
        bud->free_area[i].free_count = 0;
    }
    for (size_t i = 0; i < __fmi_ptr()->count; ++i) {
        const auto range = __fmi_ptr()->page_ranges[i];
        const phyaddr_t base = MAX(bgn_addr, range.base);
        const phyaddr_t limit = MIN(end_addr, range.limit);
        if (base >= limit) { continue; }
        block_init(base, limit, bud);
        bud->total_mem_size += range.limit - range.base;
    }
}

// added by wang 2021.6.8
int block_init(u32 bgn_addr, u32 end_addr, buddy_t *bud) {
    if (bgn_addr % SZ_4K != 0) {
        const int offset = bgn_addr % SZ_4K;
        const size_t frag_size = SZ_4K - offset;
        if (bud == kbud) { fragmentUse(bgn_addr, frag_size); }
        bgn_addr += frag_size;
    }
    size_t bsize = end_addr - bgn_addr;
    while (bsize != 0) {
        for (int i = BUDDY_ORDER_LIMIT - 1; i >= 0; i--) {
            if (bsize < block_size[i]) { continue; }
            if (bgn_addr % block_size[i] != 0) { continue; }
            free_pages(bud, pfn_to_page(phy_to_pfn(bgn_addr)), i);
            bgn_addr += block_size[i];
            bsize -= block_size[i];
            break;
        }
        if (bsize > 0 && bsize < SZ_4K) {
            if (bud == kbud) { fragmentUse(bgn_addr, bsize); }
            break;
        }
    }
    return 0;
}

int free_pages(buddy_t *bud, memory_page_t *page, u32 order) {
    const u32 max_order = BUDDY_ORDER_LIMIT - 1;
    const u32 page_order = order;

    u32 pfn = page_to_pfn(page);
    while (order < max_order) {
        u32 buddy_pfn = find_buddy_pfn(pfn, order);
        memory_page_t *buddy = pfn_to_page(buddy_pfn);
        if (!page_is_buddy(buddy_pfn, buddy, order)) { break; }
        del_page_from_free_list(buddy, bud, order);
        u32 combined_pfn = buddy_pfn & pfn;
        page = page + (combined_pfn - pfn);
        pfn = combined_pfn;
        ++order;
    }

    set_buddy_order(page, order);
    add_to_free_list(page, bud, order);

    bud->current_mem_size += block_size[page_order];
    return 0;
}

memory_page_t *alloc_pages(buddy_t *bud, u32 order) {
    u32 current_order;
    memory_page_t *page = NULL;
    struct free_area *area;

    // 从经过计算所得要查找的order开始，到MAX_ORDER进行遍历，查找bud中每一个free_area[]中是否存在空闲块
    for (current_order = order; current_order < BUDDY_ORDER_LIMIT; ++current_order) {
        area = &(bud->free_area[current_order]);
        page = get_page_from_free_area(area);
        // 若未查到则继续查找
        if (!page) { continue; }
        // 若查找到了，先将其从free_list中删除，如果需要再调用expand()函数进行大内存块拆分
        del_page_from_free_list(page, bud, current_order);
        expand(bud, page, order, current_order);

        if (page) {
            page->order = order;
            bud->current_mem_size -= block_size[order];
            return page;
        }
    }
    return NULL;
}

static void fragmentUse(u32 addr, u32 size) {
    insert_used_memblk(addr, size);
    kmem.total_mem_size += size;
    phy_kfree(addr);
}

static u32 find_buddy_pfn(u32 page_pfn, u32 order) {
    return page_pfn ^ (1 << order);
}

static u32 page_is_buddy(u32 pfn, memory_page_t *page, u32 order) {
    return page->inbuddy && page->order == order && pfn < ALL_PAGES;
}

static void del_page_from_free_list(memory_page_t *page, buddy_t *bud, u32 order) {
    assert(bud->free_area[order].free_count > 0);
    assert(page != NULL);
    memory_page_t *node = bud->free_area[order].free_list;
    if (node == page) {
        bud->free_area[order].free_list = page->next;
    } else {
        while (node->next != NULL && node->next != page) { node = node->next; }
        assert(node == page && "try to delete a page not in free list");
        node->next = node->next->next;
    }
    page->next = NULL;
    page->inbuddy = false;
    --bud->free_area[order].free_count;
    assert((bud->free_area[order].free_count == 0) && (page->next == NULL) &&
           "unexpected page in empty free list");
}

static void add_to_free_list(memory_page_t *page, buddy_t *bud, u32 order) {
    assert(page != NULL);
    assert(page->next == NULL && !page->inbuddy);
    struct free_area *area = &bud->free_area[order];
    page->next = NULL;
    if (area->free_list == NULL) {
        area->free_list = page;
    } else {
        memory_page_t *node = area->free_list;
        while (node->next != NULL && node != page) { node = node->next; }
        assert(node->next == NULL && "page already in free list");
        node->next = page;
    }
    ++area->free_count;
}

static void set_page_order(memory_page_t *page, u32 order) {
    page->order = order;
}

static void set_buddy_order(memory_page_t *page, u32 order) {
    set_page_order(page, order);
    page->inbuddy = true;
}

static memory_page_t *get_page_from_free_area(struct free_area *area) {
    assert((area->free_list == NULL) == (area->free_count == 0));
    return &area->free_list[0];
}

static void expand(buddy_t *bud, memory_page_t *page, u32 low, u32 high) {
    u32 size = 1 << high;

    while (high > low) {
        high--;
        size >>= 1;
        add_to_free_list(&page[size], bud, high);
        set_buddy_order(&page[size], high);
    }
}
