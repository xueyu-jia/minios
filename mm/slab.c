#include <minios/slab.h>
#include <minios/buddy.h>
#include <minios/memman.h>
#include <minios/assert.h>
#include <minios/layout.h>

#define LEFTSHIFT(n) (1 << (n))
// 将address向上(高地址)对齐,对齐标准为(2^n)
#define POW2_ROUNDUP(address, n) (((address) + LEFTSHIFT(n) - 1) & ~(LEFTSHIFT(n) - 1))

// cache数组下标从0开始，对应对象是8B。
// 该系统最大可存储2048B大小的字节，更大的直接分配一个页。所以 KMEM_CACHES_NUM
// 最大为 9
kmem_cache_t kmem_caches[KMEM_CACHES_NUM];

static u32 bitmap_check_avail(kmem_slab_t *slab);
static void *slab_alloc_object(kmem_slab_t *slab);
kmem_slab_t *kmem_cache_new_slab(kmem_cache_t *cache);
static void kmem_cache_init(kmem_cache_t *cache, const char *name, u32 objorder);
static int slab_check_empty(kmem_slab_t *slab);
static int slab_check_full(kmem_slab_t *slab);
static void delete_slab_from_old(kmem_slab_t *slab);
static void *kmem_cache_alloc_obj(kmem_cache_t *cache);
static kmem_cache_t *select_cache(int size);
static bool free_object_to_slab(kmem_slab_t *slab, u32 obj);
static void move_slab(kmem_slab_t *slab);

static int get_index(int objsize) {
    int index = 0;
    int temp = (objsize - 1) >> MIN_BUFF_ORDER;
    while (temp) {
        index++;
        temp >>= 1;
    }
    return index;
}

void init_cache() {
    unsigned int order;
    char name[CACHE_NAME_LEN] = "Buffer_";

    for (order = MIN_BUFF_ORDER; order <= MAX_BUFF_ORDER; ++order) {
        //! TODO: replace with sprintf
        int index = 7, temp = order;
        // 计算temp的位数
        int digits = 1;
        while (temp >= 10) {
            temp /= 10;
            digits++;
        }
        // 从最后一位开始填充数字
        index += digits;
        temp = order;
        do {
            name[index] = temp % 10 + '0';
            temp /= 10;
            index--;
        } while (temp > 0);
        name[8 + digits] = '\0';

        kmem_cache_init(&kmem_caches[order - MIN_BUFF_ORDER], name, order);
    }
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

    kmem_slab_t *slab = NULL;
    memory_page_t *page = pfn_to_page(phy_to_pfn(addr));
    int objsize = page->cache->objsize; // 得到对象的大小

    int flag = 0;
    int move = 0;

    int order = get_index(objsize);
    kmem_cache_t *cache = &kmem_caches[order];
    addr = ptr2u(K_PHY2LIN(addr));
    while (!flag) { // 对象所在的链表可能还没有添加到新的链表，循环查找
        for (int i = PARTIAL; i <= FULL && !flag; ++i) { // 依次查找partial链和full链
            spinlock_acquire(&(cache->list[i].lock));    // 先对每一个链上锁
            kmem_slab_t *temp = cache->list[i].head;
            while (temp) {
                if ((addr >= (u32)temp) && (addr < (u32)temp + SLAB_BLOCK_SIZE)) {
                    slab = temp;
                    move = free_object_to_slab(temp, addr);
                    flag = 1; // 找到
                    break;
                }
                temp = temp->next;
            }
            spinlock_release(&(cache->list[i].lock)); // 释放锁
        }
    }
    if (move) { move_slab(slab); }
}

/*!
 * \brief 检查 bitmap 返回第一个检查到的空闲 object
 */
static u32 bitmap_check_avail(kmem_slab_t *slab) {
    u32 obj_index = 0;
    bitmap_entry_t *bitmap = slab->bitmap; // 找到位图起始地址
    kmem_cache_t *cache = slab->cache;
    for (u32 i = 0; i < cache->bitmap_length; ++i) {
        // 当前块没有被完全分配
        if (bitmap[i] != BITMAP_FULL) {
            u32 j = 0; // j 是空闲对象在位图块中的相对地址
            while (bitmap[i] & (1 << j)) { ++j; }
            obj_index = i * BITMAP_ENTRY_BITS + j; // 计算出空闲对象的次序
            break;
        }
    }

    return obj_index;
}

static void *slab_alloc_object(kmem_slab_t *slab) {
    u32 obj_index;

    void *obj = NULL;
    obj_index = bitmap_check_avail(slab);
    bitmap_set_used(slab->bitmap, obj_index); // 位图相应位置置位
    slab->used_obj++;
    obj = ptr_offset(slab->objects,
                     obj_index * slab->cache->objsize); // 得到相应位置的地址
    return obj;                                         // 返回对象地址
}

static memory_page_t *slab_alloc_slab_page() {
    memory_page_t *page = buddy_alloc(bud, 0);
    assert(page != NULL);
    page->state = PAGESTATE_SLAB;
    return page;
}

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

/**
 * @brief 	给定 cache 分配新的 slab
 * @param 	cache 某个 cache 的指针
 * @retval 	新创建的 slab 的指针
 * @details	给 cache 创建新的 page，在 page 中初始化 slab、位图、对象
 */
kmem_slab_t *kmem_cache_new_slab(kmem_cache_t *cache) {
    kmem_slab_t *slab;

    memory_page_t *page = slab_alloc_slab_page();
    page->cache = cache;

    slab = (kmem_slab_t *)K_PHY2LIN(pfn_to_phy(page_to_pfn(page)));
    slab->cache = cache;
    slab->next = NULL;
    slab->used_obj = 0;
    slab->bitmap = (bitmap_entry_t *)ptr_offset(slab, sizeof(kmem_slab_t));
    slab->objects = (u32)ptr_offset(slab->bitmap, (cache->bitmap_length) * sizeof(bitmap_entry_t));
    slab->objects = POW2_ROUNDUP(slab->objects, cache->objorder); // 按照size对齐	lirong
    slab->type = EMPTY;

    /* 初始化位图 */
    u32 i = 0;
    while (i < cache->bitmap_length) slab->bitmap[i++] = BITMAP_EMPTY;

    return slab;
}

/*
        对每一个cache初始化

        传入参数：
                cache：要初始化的cache指针
                name: cache名字

*/
static void kmem_cache_init(kmem_cache_t *cache, const char *name, u32 objorder) {
    int obj_size = pow(objorder);
    u32 bitmap_size, free;
    unsigned int obj_count;
    free = SLAB_BLOCK_SIZE - sizeof(kmem_slab_t);

    // bitmap_size： 位图占用字节数
    // obj_count：对象个数
    obj_count = free / obj_size;               // 剩余空间中最多能存放的对象个数
    bitmap_size = calc_bitmap_size(obj_count); // 对象位图的字节数
    while (bitmap_size + obj_count * obj_size > free) {
        obj_count--;
        bitmap_size = calc_bitmap_size(obj_count);
    }

    // 给cache命名
    char *temp = cache->name;
    while ((*temp++ = *name++));
    //! 等价于
    //! for (; *temp != '\0'; temp++, name++) {
    //! *temp = *name;
    //:} // 师兄的写法有些难懂

    cache->objsize = obj_size;
    cache->bitmap_length = bitmap_size / sizeof(bitmap_entry_t); // 位图块数
    cache->obj_per_slab = obj_count;                             // 每个slab中对象个数
    // 初始化链表和锁
    for (int i = EMPTY; i <= FULL; ++i) {
        cache->list[i].head = NULL;
        spinlock_init(&(cache->list[i].lock), NULL);
    }
    cache->slab_count[EMPTY] = cache->slab_count[PARTIAL] = cache->slab_count[FULL] =
        0; // 统计slab数目
    cache->objorder = objorder;
}

/*
        检查slab是否为empty，用在释放对象之后

        传入参数：
                slab ： 已经释放的对象所在的slab的地址
        输出：
                0：不是empty
                1：是empty
*/
static int slab_check_empty(kmem_slab_t *slab) {
    if (slab->used_obj == 0) { return 1; }
    return 0;
}

/*
        检查slab是否为full，用在分配对象之后

        传入参数：
                slab ： 已经分配的对象所在的slab的地址
        输出：
                0：不是full
                1：是full
*/
static int slab_check_full(kmem_slab_t *slab) {
    if (slab->used_obj == slab->cache->obj_per_slab) return 1;
    return 0;
}

/*
        将slab从当前的链中删除

        传入参数：
                slab ： 要删除的slab的指针

*/
static void delete_slab_from_old(kmem_slab_t *slab) {
    kmem_cache_t *cache = slab->cache;
    slab_type_t type = slab->type;
    kmem_slab_t *cur, *prev;

    prev = NULL;
    cur = cache->list[type].head;

    while (cur != slab) {
        prev = cur;
        cur = cur->next;
        if (cur == NULL) return;
    }

    if (prev) {
        prev->next = cur->next;
    } else {
        cache->list[type].head = cur->next;
    }

    cache->slab_count[type]--;
}

/*!
 * \brief alloc object from slab cache
 *
 * \note 分配后要对 slab 的类型进行判断，按照规则将 slab 从某个链中移除，并且插入到新的链中
 */
static void *kmem_cache_alloc_obj(kmem_cache_t *cache) {
    void *obj = NULL;
    kmem_slab_t *slab = NULL;

    bool should_move = false;

    do {
        bool done = false;

        spinlock_acquire(&(cache->list[PARTIAL].lock));
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
        spinlock_release(&(cache->list[PARTIAL].lock));

        if (done) { break; }

        spinlock_acquire(&(cache->list[EMPTY].lock));
        if (cache->slab_count[EMPTY] > 0) {
            slab = cache->list[EMPTY].head;
            obj = slab_alloc_object(slab);
            assert(obj != NULL);
            done = true;
            delete_slab_from_old(slab);
            slab->type = PARTIAL;
            should_move = true;
        }
        spinlock_release(&(cache->list[EMPTY].lock));

        if (done) { break; }

        slab = kmem_cache_new_slab(cache);
        assert(slab != NULL);
        obj = slab_alloc_object(slab);
        assert(obj != NULL);
        slab->type = PARTIAL;
        should_move = true;
    } while (0);

    if (should_move) { move_slab(slab); }

    return obj;
}

/*!
 * \brief 根据要分配的对象大小，选择从哪个 cache 分配对象
 */
static kmem_cache_t *select_cache(int size) {
    int index = get_index(size);
    return &(kmem_caches[index]);
}

/*!
 * \brief 将一个对象释放到它所在的 page 中
 * \return slab 是否从当前链表删除
 *
 * \note 同时判断释放后的 slab 类型，对满足规则的 slab 要从原来
 * 链表中删除，并且修改 slab 类型为它现在的类型
 */
static bool free_object_to_slab(kmem_slab_t *slab, u32 obj) {
    u32 obj_index;                  // 要释放的对象在slab中的次序
    u32 start_addr = slab->objects; // 对象在slab中的起始地址
    obj_index = (obj - start_addr) / (slab->cache->objsize);
    if (bitmap_is_free(slab->bitmap, obj_index)) { return 0; }
    bitmap_set_free(slab->bitmap, obj_index);
    slab->used_obj--;

    if (slab->type == PARTIAL) {      // slab 释放之前是 PARTIAL 类型
        if (slab_check_empty(slab)) { // 释放后是 EMPTY 类型
            delete_slab_from_old(slab);
            slab->type = EMPTY;
            return true;
        }
    } else if (slab->type == FULL) { // slab 释放之前是 FULL 类型
        // 将slab从原来链中移除，移动到新的slab链中
        delete_slab_from_old(slab);
        slab->type = PARTIAL;
        return true;
    }

    return false;
}

/*!
 * \brief 将 slab 插入到对应类型的链表上
 */
static void move_slab(kmem_slab_t *slab) {
    kmem_cache_t *cache = slab->cache;
    slab_type_t type = slab->type;

    // empty 链上的 page 个数不能超过最大值，多的需要被释放
    if (type == EMPTY && slab->cache->slab_count[EMPTY] >= MAX_EMPTY) {
        slab_free_slab_page(slab);
        return;
    }

    spinlock_acquire(&cache->list[type].lock);
    slab->next = cache->list[type].head;
    cache->list[type].head = slab;
    ++cache->slab_count[type];
    spinlock_release(&cache->list[type].lock);
}
