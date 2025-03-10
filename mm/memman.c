#include <minios/memman.h>
#include <minios/buddy.h>
#include <minios/kmalloc.h>
#include <minios/mempage.h>
#include <minios/page.h>
#include <minios/mmap.h>
#include <minios/console.h>
#include <minios/shm.h>
#include <minios/layout.h>
#include <minios/assert.h>
#include <minios/spinlock.h>
#include <minios/syscall.h>
#include <minios/kstate.h>
#include <fs/fs.h>
#include <klib/stddef.h>
#include <klib/size.h>
#include <klib/atomic.h>
#include <string.h>

bool big_kernel = 0;
u32 kernel_size = 0;      // 表示内核大小的全局变量
u32 kernel_code_size = 0; // 为内核代码数据分配的内存大小

memory_page_t *mem_map = NULL;
size_t nr_mmap_pages = 0;

list_head page_inactive = {
    .next = &page_inactive,
    .prev = &page_inactive,
};
static spinlock_t page_inactive_lock = {};

static void stat_buddy_pages(const char *name, const buddy_t *bud) {
    for (int i = 0; i < BUDDY_ORDER_LIMIT; ++i) {
        const auto area = bud->free_area[i];
        if (area.free_count == 0) { continue; }
        kprintf("%s order=%02d (%02d)", name, i, area.free_count);
        const auto size = buddy_order_size[i];
        memory_page_t *node = area.free_list;
        while (node) {
            const auto pa = pfn_to_phy(page_to_pfn(node));
            kprintf(" 0x%p~0x%p", pa, pa + size);
            node = node->next;
        }
        kprintf("\n");
    }
}

static void mark_as_critical(phyaddr_t pa, size_t size) {
    if (size == 0) { return; }
    const phyaddr_t base = ROUNDDOWN(pa, PGSIZE);
    const phyaddr_t limit = ROUNDUP(pa + size, PGSIZE);
    const int first_pfn = phy_to_pfn(base);
    const int last_pfn = phy_to_pfn(limit) - 1;
    for (int i = first_pfn; i <= last_pfn; ++i) {
        if (mem_map[i].state == PAGESTATE_CRITICAL) { continue; }
        memset(&mem_map[i], 0, sizeof(memory_page_t));
        mem_map[i].state = PAGESTATE_CRITICAL;
    }
}

void memory_init() {
    kprintf("info: init memory management system\n");

    assert(!!(kstate_mbi->flags & BIT(6)) && "no mmap found in multiboot info");
    multiboot_mmap_entry_t *mmap_ent = K_PHY2LIN(kstate_mbi->mmap_addr);
    multiboot_mmap_entry_t *mmap_limit = (void *)mmap_ent + kstate_mbi->mmap_length;

    phyaddr_t highest_page_addr = 0;
    {
        auto ent = mmap_ent;
        while (ent < mmap_limit) {
            const phyaddr_t pa_limit = ROUNDDOWN(ent->base_addr + ent->length, PGSIZE);
            highest_page_addr = MAX(highest_page_addr, pa_limit);
            ent = (void *)ent + ent->size + sizeof(ent->size);
        }
    }
    const size_t page_limit = highest_page_addr / PGSIZE;

    const size_t mm_mmap_size = sizeof(memory_page_t) * page_limit;
    mem_map = NULL;
    {
        auto ent = mmap_ent;
        while (ent < mmap_limit) {
            const phyaddr_t limit = ent->base_addr + ent->length;
            if (limit > SZ_1M && ent->length > mm_mmap_size) {
                mem_map = K_PHY2LIN(ent->base_addr);
                const phyaddr_t old_addr = ent->base_addr;
                ent->base_addr = ROUNDUP(ent->base_addr + mm_mmap_size, PGSIZE);
                ent->length -= ent->base_addr - old_addr;
                break;
            }
            ent = (void *)ent + ent->size + sizeof(ent->size);
        }
    }
    assert(mem_map != NULL && "failed to build memory page table");
    nr_mmap_pages = page_limit;
    kprintf("info: build memory map with %d pages at 0x%p~0x%p\n", nr_mmap_pages,
            K_LIN2PHY(mem_map), K_LIN2PHY(mem_map + nr_mmap_pages));

    //! NOTE: it can be proved that mem_map will never exceed our mapped pages
    //! ATTENTION: it depends on the bootloader to map the whole kernel space at here
    memset(mem_map, 0, sizeof(memory_page_t) * nr_mmap_pages);
    {
        auto ent = mmap_ent;
        while (ent < mmap_limit) {
            const phyaddr_t base = ROUNDUP(ent->base_addr, PGSIZE);
            const phyaddr_t limit = ROUNDDOWN(ent->base_addr + ent->length, PGSIZE);
            const int first_pfn = phy_to_pfn(base);
            const int last_pfn = phy_to_pfn(limit) - 1;
            for (int i = first_pfn; i <= last_pfn; ++i) { mem_map[i].state = PAGESTATE_PHANTOM; }
            ent = (void *)ent + ent->size + sizeof(ent->size);
        }
    }

    //! mark critical pages
    //! 1. vector table + rom bios param block
    assert(mem_map[0].state == PAGESTATE_PHANTOM);
    mark_as_critical(0, 0x400 + 0x100);
    //! 2. mem_map
    mark_as_critical(K_LIN2PHY(mem_map), mm_mmap_size);
    //! 3. kernel code
    //! TODO: get elf info from multiboot info
    //! 4. multiboot info
    mark_as_critical(K_LIN2PHY(kstate_mbi), sizeof(multiboot_boot_info_t));
    mark_as_critical(kstate_mbi->mmap_addr, kstate_mbi->mmap_length);
    //! TODO: extra multiboot info

    //! TODO: better solution to mark critical pages
    //! NOTE: patch that exclude vector table from available pages
    assert(mmap_ent < mmap_limit && mmap_ent[0].base_addr == 0 && mmap_ent[0].length > SZ_4K);
    mmap_ent[0].base_addr = SZ_4K;
    mmap_ent[0].length -= SZ_4K;

    //! init kernel info
    //! FIXME: init with elf info
    big_kernel = true;
    kernel_code_size = SZ_8M;
    kernel_size = SZ_32M;

    //! init buddy system for normal region
    for (int i = 0; i < BUDDY_ORDER_LIMIT; ++i) { buddy_order_size[i] = SZ_4K << i; }
    buddy_init(bud, 0, highest_page_addr);
    stat_buddy_pages("bud", bud);

    //! init slab system
    init_cache();
}

phyaddr_t phy_kmalloc(size_t size) {
    if (size <= BIT(MAX_BUFF_ORDER)) { return slab_alloc(size); }
    memory_page_t *page = buddy_alloc_restricted(bud, buddy_fit_order(size), 0, SZ_1G - 1);
    if (page == NULL) { return PHY_NULL; }
    const ssize_t page_size = page->size_hint * PGSIZE;
    const phyaddr_t pa = pfn_to_phy(page_to_pfn(page));
    if (page_size - size >= PGSIZE) {
        buddy_join_blk(bud, pa + size, pa + page_size, false);
        page->size_hint = ROUNDUP(size, PGSIZE) / PGSIZE;
    }
    return pa;
}

void phy_kfree(phyaddr_t phy_addr) {
    const int pfn = phy_to_pfn(ROUNDDOWN(phy_addr, PGSIZE));
    assert(pfn >= 0 && pfn < (ssize_t)nr_mmap_pages);
    memory_page_t *page = pfn_to_page(pfn);
    switch (page->state) {
        case PAGESTATE_SLAB: {
            slab_free(phy_addr);
        } break;
        case PAGESTATE_ALLOCATED: {
            buddy_free(bud, page);
        } break;
        default: {
            unreachable();
        } break;
    }
}

phyaddr_t phy_malloc_4k() {
    memory_page_t *page = buddy_alloc(bud, 0);
    if (page == NULL) { return PHY_NULL; }
    return pfn_to_phy(page_to_pfn(page));
}

void phy_free_4k(phyaddr_t phy_addr) {
    assert(phy_addr % PGSIZE == 0);
    const int pfn = phy_to_pfn(phy_addr);
    assert(pfn >= 0 && pfn < (ssize_t)nr_mmap_pages);
    memory_page_t *page = pfn_to_page(pfn);
    assert(page->state == PAGESTATE_ALLOCATED && page->size_hint == BIT(0));
    assert(atomic_get(&page->count) == 0);
    return buddy_free(bud, page);
}

void *kern_kmalloc(size_t size) {
    return K_PHY2LIN(phy_kmalloc(size));
}

void *kern_kzalloc(size_t size) {
    void *ptr = kern_kmalloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void kern_kfree(void *ptr) {
    phy_kfree(K_LIN2PHY(ptr));
}

void *kern_malloc_4k() {
    void *va = u2ptr(get_heap_limit(p_proc_current->task.pid));
    update_heap_limit(p_proc_current->task.pid, 1);
    const int retval =
        kern_mmap_safe(ptr2u(va), PGSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    if (retval == -1) {
        update_heap_limit(p_proc_current->task.pid, -1);
        kprintf("kern_malloc_4k: error mmap addr already occupied\n");
        return NULL;
    }
    assert(u2ptr(retval) == va);
    return va;
}

void kern_free_4k(void *ptr) {
    if (ptr == NULL) { return; }
    update_heap_limit(p_proc_current->task.pid, -1);
    const int retval = kern_munmap(p_proc_current, (u32)ptr, PGSIZE);
    assert(retval == 0);
}

size_t kern_total_mem_size() {
    return bud->total_mem - bud->used_mem;
}

// mmu for user vmem addr

static int drop_page(memory_page_t *page) {
    address_space_t *mapping = page->pg_mapping;
    if (mapping) {
        spinlock_lock_or_yield(&mapping->lock);
        const int retval = free_mem_page(page);
        spinlock_release(&mapping->lock);
        return retval;
    }
    assert(page->state == PAGESTATE_ALLOCATED && page->size_hint == BIT(0));
    page->user_va = NULL;
    buddy_free(bud, page);
    return 0;
}

void get_page(memory_page_t *page) {
    if (atomic_get(&page->count) == 0) {
        spinlock_acquire(&page_inactive_lock);
        list_remove(&page->pg_lru);
        spinlock_release(&page_inactive_lock);
    }
    atomic_inc(&page->count);
}

int put_page(memory_page_t *page) {
    if (!atomic_dec_and_test(&page->count)) { return 0; }
    const int type = page->pg_mapping ? page->pg_mapping->type : MEMPAGE_AUTO;
    switch (type) {
        case MEMPAGE_AUTO: {
            return drop_page(page);
        } break;
        case MEMPAGE_CACHED: {
            // 通常是文件映射的页面，将 page 插入 RLU
            // 此页面暂时不再使用，可能在未来内存不足时释放
            spinlock_acquire(&page_inactive_lock);
            list_add_first(&page->pg_lru, &page_inactive);
            spinlock_release(&page_inactive_lock);
            return 0;
        } break;
        case MEMPAGE_SAVE: {
            // 此页面暂时不再使用，由其他组件手动释放
            return 0;
        } break;
    }
    return -1;
}

static memory_page_t *get_and_remove_inactive_page() {
    spinlock_acquire(&page_inactive_lock);
    memory_page_t *page = list_back(&page_inactive, memory_page_t, pg_lru);
    list_remove(&page->pg_lru);
    spinlock_release(&page_inactive_lock);
    return page;
}

/*!
 * \brief find the first overlapping vma
 *
 * [addr, addr+size) may covers multiple vma, we only need to find the first one, for this reason,
 * we don't care the size of memory block here
 *
 * \note requires pcb.memmap.vma_lock
 */
struct vmem_area *find_vma(memmap_t *mmap, u32 addr) {
    struct vmem_area *vma = NULL;
    list_for_each(&mmap->vma_map, vma, vma_list) {
        if (addr >= vma->start && addr < vma->end) { return vma; }
    }
    return NULL;
}

memory_page_t *alloc_user_page(u32 pgoff) {
    if (bud->total_mem - bud->used_mem < bud->total_mem / 16) {
        //! shrink page memory
        const int max_shrink = 16;
        int nr_shrink = 0;
        // 因为上锁顺序的限制，持有 page_inactive_lock 不能再获取 mapping 的锁，防死锁
        while (nr_shrink < max_shrink) {
            memory_page_t *page = get_and_remove_inactive_page();
            if (!page) { break; }
            const bool ok = drop_page(page);
            assert(ok);
            ++nr_shrink;
        }
    }
    memory_page_t *page = buddy_alloc(bud, 0);
    atomic_set(&page->count, 1);
    page->pg_off = pgoff;
    page->pg_mapping = NULL;
    list_init(&page->pg_list);
    list_init(&page->pg_lru);
    memset(page->pg_buffer, 0, sizeof(page->pg_buffer));
    page->user_va = kmap(page);
    return page;
}

// req. ring 0
void copy_page(memory_page_t *dst, memory_page_t *src) {
    void *va_dst = dst->user_va ? dst->user_va : kpage_lin(dst);
    void *va_src = src->user_va ? src->user_va : kpage_lin(src);
    memcpy(va_dst, va_src, PGSIZE);
}

// req. ring 0
void zero_page(memory_page_t *page) {
    void *ptr = page->user_va ? page->user_va : kpage_lin(page);
    memset(ptr, 0, PGSIZE);
}

// req. ring 0
void copy_from_page(memory_page_t *page, void *buf, u32 len, u32 offset) {
    void *src = page->user_va ? page->user_va : kpage_lin(page);
    memcpy(buf, src + offset, len);
}

// req. ring 0
void copy_to_page(memory_page_t *page, const void *buf, u32 len, u32 offset) {
    void *dst = page->user_va ? page->user_va : kpage_lin(page);
    memcpy(dst + offset, buf, len);
}
