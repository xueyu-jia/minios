#include <minios/buddy.h>
#include <minios/kmalloc.h>
#include <minios/assert.h>
#include <minios/kstate.h>
#include <minios/memman.h>
#include <klib/size.h>
#include <compiler.h>
#include <string.h>

//! FIXME: spinlock for buddy system would cause deadlock in fs testcases
#if 1
#define might_deadlock_lock(spinlock) spinlock_lock_or_yield(spinlock)
#define might_deadlock_unlock(spinlock) spinlock_release(spinlock)
#else
#define might_deadlock_lock(...)
#define might_deadlock_unlock(...)
#endif

static buddy_t buddy = {};
buddy_t *bud = &buddy;

size_t block_size[BUDDY_ORDER_LIMIT];

static int get_buddy_pfn(int pfn, int order) {
    return pfn ^ (1 << order);
}

static void buddy_set_page(memory_page_t *page, int order) {
    assert(page != NULL);
    assert(order == -1 || (order >= 0 && order < BUDDY_ORDER_LIMIT));
    page->state = PAGESTATE_BUDDY;
    page->order = order;
}

static void buddy_join_page(buddy_t *bud, memory_page_t *page, int order) {
    assert(bud != NULL);
    assert(order >= 0 && order < BUDDY_ORDER_LIMIT);
    assert(page != NULL);
    const int pfn = page_to_pfn(page);
    assert(pfn >= 0 && pfn < (ssize_t)nr_mmap_pages);
    assert(page->state == PAGESTATE_PHANTOM);
    assert(page->next == NULL);

    struct free_area *area = &bud->free_area[order];
    if (area->free_list == NULL) {
        area->free_list = page;
    } else {
        memory_page_t *node = area->free_list;
        while (node->next != NULL && node != page) { node = node->next; }
        assert(node->next == NULL && "page already in free list");
        node->next = page;
    }
    ++area->free_count;
    buddy_set_page(page, order);
}

static void buddy_remove_page(buddy_t *bud, memory_page_t *page) {
    assert(bud != NULL);
    assert(page != NULL);
    const int pfn = page_to_pfn(page);
    assert(pfn >= 0 && pfn < (ssize_t)nr_mmap_pages);
    assert(page->state == PAGESTATE_BUDDY);
    assert(page->order >= 0 && page->order < BUDDY_ORDER_LIMIT);
    assert(bud->free_area[page->order].free_count > 0);

    struct free_area *area = &bud->free_area[page->order];
    if (area->free_list == page) {
        area->free_list = page->next;
    } else {
        memory_page_t *node = area->free_list;
        while (node->next != NULL && node->next != page) { node = node->next; }
        assert(node->next == page && "try to delete a page not in free list");
        node->next = node->next->next;
    }
    --area->free_count;
    memset(page, 0, sizeof(memory_page_t));
    page->state = PAGESTATE_PHANTOM;
}

static void buddy_join_phantom_pages(buddy_t *bud, int pfn, int order, bool strict) {
    assert(bud != NULL);
    assert(order >= 0 && order < BUDDY_ORDER_LIMIT);
    const ssize_t nr_pages = block_size[order] / PGSIZE;
    assert(pfn >= 0 && pfn + nr_pages <= (ssize_t)nr_mmap_pages);
    memory_page_t *const blk_head = pfn_to_page(pfn);
    if (strict) {
        assert(blk_head->state == PAGESTATE_PHANTOM);
        for (int i = 1; i < nr_pages; ++i) {
            assert(blk_head[i].state == PAGESTATE_PHANTOM);
            //! NOTE: order of a non-direct page is marked as -1
            buddy_set_page(&blk_head[i], -1);
        }
    }
    buddy_join_page(bud, blk_head, order);
    fetch_add(&bud->total_mem, block_size[order]);
}

ssize_t buddy_join_blk(buddy_t *bud, phyaddr_t pa_lo, phyaddr_t pa_hi, bool strict) {
    assert(bud != NULL);

    pa_lo = ROUNDUP(pa_lo, PGSIZE);
    pa_hi = ROUNDDOWN(pa_hi, PGSIZE);

    might_deadlock_lock(&bud->lock);
    const ssize_t old_total_mem = bud->total_mem;
    while (pa_lo < pa_hi) {
        for (int i = BUDDY_ORDER_LIMIT - 1; i >= 0; --i) {
            if (pa_lo + block_size[i] > pa_hi) { continue; }
            if (pa_lo % block_size[i] != 0) { continue; }
            buddy_join_phantom_pages(bud, phy_to_pfn(pa_lo), i, strict);
            pa_lo += block_size[i];
            break;
        }
    }
    const ssize_t total_mem_inc = bud->total_mem - old_total_mem;
    might_deadlock_unlock(&bud->lock);

    return total_mem_inc;
}

void buddy_init(buddy_t *bud, phyaddr_t pa_lo, phyaddr_t pa_hi) {
    assert(bud != NULL);

    memset(bud, 0, sizeof(buddy_t));
    //! TODO: use detailed name
    spinlock_init(&bud->lock, "buddy");

    if (pa_lo >= pa_hi) { return; }

    multiboot_mmap_entry_t *mmap_ent = K_PHY2LIN(kstate_mbi->mmap_addr);
    multiboot_mmap_entry_t *mmap_limit = (void *)mmap_ent + kstate_mbi->mmap_length;

    auto ent = mmap_ent;
    while (ent < mmap_limit) {
        const phyaddr_t base = MAX(pa_lo, (phyaddr_t)ent->base_addr);
        const phyaddr_t limit = MIN(pa_hi, (phyaddr_t)(ent->base_addr + ent->length));
        do {
            if (base >= limit) { break; }
            buddy_join_blk(bud, base, limit, true);
        } while (0);
        ent = (void *)ent + ent->size + sizeof(ent->size);
    }
}

int buddy_fit_order(size_t size) {
    for (int order = 0; order < BUDDY_ORDER_LIMIT; ++order) {
        if (size <= block_size[order]) { return order; }
    }
    unreachable();
}

memory_page_t *buddy_alloc(buddy_t *bud, int order) {
    assert(bud != NULL);
    assert(order >= 0 && order < BUDDY_ORDER_LIMIT);

    memory_page_t *page = NULL;

    might_deadlock_lock(&bud->lock);
    for (int i = order; i < BUDDY_ORDER_LIMIT; ++i) {
        struct free_area *area = &bud->free_area[i];
        page = area->free_list;
        if (page == NULL) { continue; }
        buddy_remove_page(bud, page);
        for (int j = i - 1; j >= order; --j) {
            //! NOTE: patch the state to trick the buddy_join_page(...)
            memory_page_t *page_recycle = page + BIT(j);
            page_recycle->state = PAGESTATE_PHANTOM;
            buddy_join_page(bud, page_recycle, j);
        }
        page->state = PAGESTATE_ALLOCATED;
        page->size_hint = BIT(order);
        bud->used_mem += block_size[order];
        break;
    }
    might_deadlock_unlock(&bud->lock);

    return page;
}

void buddy_free(buddy_t *bud, memory_page_t *blk_head) {
    assert(bud != NULL);
    assert(blk_head != NULL);
    const int pfn = page_to_pfn(blk_head);
    assert(pfn >= 0 && pfn < (ssize_t)nr_mmap_pages);
    assert(blk_head->state > PAGESTATE_BUDDY && blk_head->state < NR_PAGESTATE);

    const u32 blk_bits = blk_head->size_hint;
    assert(blk_bits > 0 && blk_bits <= BIT(BUDDY_ORDER_LIMIT - 1));

    int lo_order = BUDDY_ORDER_LIMIT;
    int hi_order = -1;
    for (int order = 0; order < BUDDY_ORDER_LIMIT; ++order) {
        if (!(blk_bits & BIT(order))) { continue; }
        hi_order = MAX(hi_order, order);
        lo_order = MIN(lo_order, order);
    }
    const int origin_order = !!(blk_bits & ~BIT(hi_order)) ? hi_order + 1 : hi_order;
    assert((pfn & (BIT(origin_order) - 1)) == 0);

    might_deadlock_lock(&bud->lock);
    for (int order = 0; order + 1 < BUDDY_ORDER_LIMIT; ++order) {
        if (!(blk_bits & BIT(order))) { continue; }
        const int cur_blk_pfn = pfn + (blk_bits & (BIT(order) - 1));
        memory_page_t *cur_blk = pfn_to_page(cur_blk_pfn);
        assert(cur_blk == blk_head || (cur_blk->state == PAGESTATE_BUDDY && cur_blk->order == -1));
        buddy_set_page(cur_blk, -1);
        int cur_pfn = cur_blk_pfn;
        int cur_order = order;
        while (cur_order + 1 < BUDDY_ORDER_LIMIT) {
            const int buddy_pfn = get_buddy_pfn(cur_pfn, cur_order);
            if (buddy_pfn >= (ssize_t)nr_mmap_pages) { break; }
            memory_page_t *buddy_page = pfn_to_page(buddy_pfn);
            if (buddy_page->state != PAGESTATE_BUDDY || buddy_page->order != cur_order) { break; }
            buddy_remove_page(bud, buddy_page);
            buddy_set_page(buddy_page, -1);
            cur_pfn &= buddy_pfn;
            ++cur_order;
        }
        memory_page_t *recycle_page = pfn_to_page(cur_pfn);
        recycle_page->state = PAGESTATE_PHANTOM;
        buddy_join_page(bud, recycle_page, order);
    }
    if (hi_order + 1 == BUDDY_ORDER_LIMIT) {
        blk_head->state = PAGESTATE_PHANTOM;
        buddy_join_page(bud, blk_head, hi_order);
    }
    bud->used_mem -= block_size[origin_order];
    might_deadlock_unlock(&bud->lock);
}
