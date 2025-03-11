#include <minios/slab.h>
#include <minios/buddy.h>
#include <minios/memman.h>
#include <minios/assert.h>
#include <minios/layout.h>
#include <minios/console.h>
#include <string.h>
#include <fmt.h>

#define ptr_offset(ptr, offset) (void *)((char *)ptr + offset)

#define bitmap_set_used(bitmap, index) \
    bitmap[index / BITMAP_ENTRY_BITS] |= (1 << (index % BITMAP_ENTRY_BITS))
#define bitmap_set_free(bitmap, index) \
    bitmap[index / BITMAP_ENTRY_BITS] &= ~(1 << (index % BITMAP_ENTRY_BITS))
#define bitmap_is_free(bitmap, index) \
    ((bitmap[index / BITMAP_ENTRY_BITS] & (1 << (index % BITMAP_ENTRY_BITS))) == 0)

kmem_cache_t kmem_caches[KMEM_CACHES_NUM];

static int slab_get_index(int obj_size) {
    int index = 0;
    int temp = (obj_size - 1) >> MIN_BUFF_ORDER;
    while (temp) {
        index++;
        temp >>= 1;
    }
    return index;
}

static void kmem_cache_init(kmem_cache_t *cache, const char *name, size_t obj_order) {
    const size_t obj_size = BIT(obj_order);

    size_t free_space = SLAB_BLOCK_SIZE - sizeof(kmem_slab_t);
    assert(free_space >= sizeof(bitmap_entry_t) + obj_size);

    int nr_bitmap = free_space / (sizeof(bitmap_entry_t) + BITMAP_ENTRY_BITS * obj_size);
    int nr_objs = nr_bitmap * BITMAP_ENTRY_BITS;
    free_space -= nr_bitmap * (sizeof(bitmap_entry_t) + BITMAP_ENTRY_BITS * obj_size);
    if (free_space >= sizeof(bitmap_entry_t) + obj_size) {
        ++nr_bitmap;
        nr_objs += (free_space - sizeof(bitmap_entry_t)) / obj_size;
    }

    strncpy(cache->name, name, sizeof(cache->name));

    cache->obj_order = obj_order;
    cache->obj_size = obj_size;
    cache->nr_bitmap = nr_bitmap;
    cache->obj_per_slab = nr_objs;

    const int chain_ty[] = {EMPTY, PARTIAL, FULL};
    for (int i = 0; i < ARRAY_SIZE(chain_ty); ++i) {
        cache->list[chain_ty[i]].head = NULL;
        spinlock_init(&cache->list[chain_ty[i]].lock, NULL);
        cache->slab_count[chain_ty[i]] = 0;
    }
}

void init_cache() {
    char name[CACHE_NAME_LEN] = {};
    for (int order = MIN_BUFF_ORDER; order <= MAX_BUFF_ORDER; ++order) {
        nstrfmt(name, sizeof(name), "slab,%d", order);
        kmem_cache_init(&kmem_caches[order - MIN_BUFF_ORDER], name, order);
    }
}

/*!
 * \brief select the best-fit cache for the requested size
 */
static kmem_cache_t *select_cache(int size) {
    assert(size >= 0 && size <= BIT(MAX_BUFF_ORDER));
    return &kmem_caches[slab_get_index(size)];
}

/*!
 * \brief whether a slab is empty
 */
static int slab_check_empty(kmem_slab_t *slab) {
    return slab->used_obj == 0;
}

/*!
 * \brief whether a slab is full
 */
static int slab_check_full(kmem_slab_t *slab) {
    return slab->used_obj == slab->cache->obj_per_slab;
}

/*!
 * \brief get index of first available object from bitmap
 */
static int bitmap_check_avail(kmem_slab_t *slab) {
    const int nr_bitmap = slab->cache->nr_bitmap;
    for (int i = 0; i < nr_bitmap; ++i) {
        if (slab->bitmap[i] == BITMAP_FULL) { continue; }
        size_t j = 0;
        while (slab->bitmap[i] & BIT(j)) { ++j; }
        if (j >= BITMAP_ENTRY_BITS) { continue; }
        const size_t index = i * BITMAP_ENTRY_BITS + j;
        if (index < slab->cache->obj_per_slab) { return index; }
        assert(i + 1 == nr_bitmap);
    }
    return -1;
}

/*!
 * \brief remove slab from current slab chain
 */
static void delete_slab_from_old(kmem_slab_t *slab) {
    kmem_cache_t *cache = slab->cache;
    slab_type_t type = slab->type;

    kmem_slab_t *prev = NULL;
    kmem_slab_t *cur = cache->list[type].head;

    while (cur != slab) {
        prev = cur;
        cur = cur->next;
        if (cur == NULL) { return; }
    }

    if (prev) {
        prev->next = cur->next;
    } else {
        cache->list[type].head = cur->next;
    }

    --cache->slab_count[type];
}

static void *slab_alloc_object(kmem_slab_t *slab) {
    const int index = bitmap_check_avail(slab);
    assert(index != -1);
    bitmap_set_used(slab->bitmap, index);
    ++slab->used_obj;
    return ptr_offset(slab->objects, index * slab->cache->obj_size);
}

/*!
 * \brief free object to slab
 * \return whther the target slab is removed from its old slab chain
 */
static bool free_object_to_slab(kmem_slab_t *slab, u32 obj) {
    assert((obj - slab->objects) % slab->cache->obj_size == 0);
    const int obj_index = (obj - slab->objects) / slab->cache->obj_size;
    if (bitmap_is_free(slab->bitmap, obj_index)) { return 0; }

    bitmap_set_free(slab->bitmap, obj_index);
    --slab->used_obj;

    if (slab->type == FULL) {
        delete_slab_from_old(slab);
        slab->type = PARTIAL;
        return true;
    }

    if (slab->type == PARTIAL && slab_check_empty(slab)) {
        delete_slab_from_old(slab);
        slab->type = EMPTY;
        return true;
    }

    return false;
}

/*!
 * \brief alloc a memory page for slab ctor
 */
static memory_page_t *slab_alloc_slab_page() {
    memory_page_t *page = buddy_alloc_restricted(bud, 0, 0, SZ_1G - 1);
    assert(page != NULL);
    page->state = PAGESTATE_SLAB;
    return page;
}

/*!
 * \brief free slab and release the page
 */
static void slab_free_slab_page(kmem_slab_t *slab) {
    const phyaddr_t pa = K_LIN2PHY(slab);
    assert(pa % PGSIZE == 0);
    const int pfn = phy_to_pfn(pa);
    assert(pfn >= 0 && pfn < (ssize_t)nr_mmap_pages);
    memory_page_t *page = pfn_to_page(pfn);
    assert(page->state == PAGESTATE_SLAB);
    page->state = PAGESTATE_ALLOCATED;
    buddy_free(bud, page);
}

/*!
 * \brief move slab to target slab chain
 */
static void move_slab(kmem_slab_t *slab) {
    kmem_cache_t *cache = slab->cache;
    slab_type_t type = slab->type;

    if (type == EMPTY && cache->slab_count[EMPTY] >= MAX_EMPTY) {
        slab_free_slab_page(slab);
        return;
    }

    spinlock_acquire(&cache->list[type].lock);
    slab->next = cache->list[type].head;
    cache->list[type].head = slab;
    ++cache->slab_count[type];
    spinlock_release(&cache->list[type].lock);
}

/*!
 * \brief alloc new slab to the cache
 */
kmem_slab_t *kmem_cache_new_slab(kmem_cache_t *cache) {
    memory_page_t *page = slab_alloc_slab_page();
    assert(page->size_hint * SZ_4K == SLAB_BLOCK_SIZE);
    page->cache = cache;

    const size_t obj_pool_size = cache->obj_size * cache->obj_per_slab;

    kmem_slab_t *slab = kpage_lin(page);
    slab->cache = cache;
    slab->next = NULL;
    slab->used_obj = 0;
    slab->bitmap = ptr_offset(slab, sizeof(kmem_slab_t));
    slab->objects = ptr2u(ptr_offset(slab, SLAB_BLOCK_SIZE - obj_pool_size));
    assert((void *)&slab->bitmap[cache->nr_bitmap + 1] <= u2ptr(slab->objects));
    assert(slab->objects % cache->obj_size == 0);

    for (size_t i = 0; i < cache->nr_bitmap; ++i) { slab->bitmap[i] = BITMAP_EMPTY; }
    slab->type = EMPTY;

    return slab;
}

/*!
 * \brief alloc object from slab cache
 */
static void *kmem_cache_alloc_obj(kmem_cache_t *cache) {
    void *obj = NULL;
    kmem_slab_t *slab = NULL;

    bool should_move = false;

    do {
        bool done = false;

        spinlock_acquire(&cache->list[PARTIAL].lock);
        if (cache->slab_count[PARTIAL] > 0) {
            slab = cache->list[PARTIAL].head;
            obj = slab_alloc_object(slab);
            assert(obj != NULL);
            done = true;
            if (slab_check_full(slab)) {
                delete_slab_from_old(slab);
                slab->type = FULL;
                should_move = true;
            }
        }
        spinlock_release(&cache->list[PARTIAL].lock);

        if (done) { break; }

        spinlock_acquire(&cache->list[EMPTY].lock);
        if (cache->slab_count[EMPTY] > 0) {
            slab = cache->list[EMPTY].head;
            obj = slab_alloc_object(slab);
            assert(obj != NULL);
            done = true;
            delete_slab_from_old(slab);
            slab->type = PARTIAL;
            should_move = true;
        }
        spinlock_release(&cache->list[EMPTY].lock);

        if (done) { break; }

        slab = kmem_cache_new_slab(cache);
        assert(slab != NULL);
        obj = slab_alloc_object(slab);
        assert(obj != NULL);
        slab->type = slab->used_obj == slab->cache->obj_per_slab ? FULL : PARTIAL;
        should_move = true;
    } while (0);

    if (should_move) { move_slab(slab); }

    return obj;
}

phyaddr_t slab_alloc(size_t size) {
    kmem_cache_t *cache = select_cache(size);
    if (cache == NULL) { return PHY_NULL; }
    void *ptr = kmem_cache_alloc_obj(cache);
    if (ptr == NULL) { return PHY_NULL; }
    return K_LIN2PHY(ptr);
}

void slab_free(phyaddr_t addr) {
    if (addr == PHY_NULL) { return; }

    const memory_page_t *page = pfn_to_page(phy_to_pfn(addr));
    const int order = slab_get_index(page->cache->obj_size);
    kmem_cache_t *cache = &kmem_caches[order];
    const uintptr_t va = ptr2u(K_PHY2LIN(addr));

    kmem_slab_t *slab = NULL;
    bool found = false;
    bool should_move = false;

    const int chains[] = {PARTIAL, FULL};
    const int max_retries = 1024;
    int retries = 0;

    while (true) {
        //! NOTE: obj's slab might be moving and not join into the its new slab chain yet, loop to
        //! wait the movement to be completed
        for (int i = 0; i < ARRAY_SIZE(chains); ++i) {
            spinlock_acquire(&cache->list[chains[i]].lock);
            slab = cache->list[chains[i]].head;
            while (slab != NULL) {
                if (va >= ptr2u(slab) && va < ptr2u(slab) + SLAB_BLOCK_SIZE) {
                    should_move = free_object_to_slab(slab, va);
                    found = true;
                    break;
                }
                slab = slab->next;
            }
            spinlock_release(&cache->list[chains[i]].lock);
            if (found) { break; }
        }
        if (found) { break; }
        if (++retries >= max_retries) {
            bool might_already_free = false;
            slab = cache->list[EMPTY].head;
            while (slab != NULL) {
                if (va >= ptr2u(slab) && va < ptr2u(slab) + SLAB_BLOCK_SIZE) {
                    might_already_free = true;
                    break;
                }
                slab = slab->next;
            }
            spinlock_release(&cache->list[EMPTY].lock);
            assert(might_already_free);
            kprintf("error: slab_free: request to free an unallocated object 0x%p\n", va);
            break;
        }
    }

    if (should_move) { move_slab(slab); }
}
