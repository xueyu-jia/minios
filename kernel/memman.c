#include <minios/buddy.h> //added by mingxuan 2021-3-8
#include <minios/fs.h>
#include <minios/kmalloc.h>
#include <minios/memman.h>
#include <minios/mempage.h>
#include <minios/mmap.h>
#include <minios/page.h>
#include <minios/proto.h>
#include <minios/shm.h>
#include <minios/slab.h>
#include <minios/assert.h>
#include <klib/size.h>
#include <string.h>

bool big_kernel = 0;
u32 kernel_size = 0;      // 表示内核大小的全局变量
u32 kernel_code_size = 0; // 为内核代码数据分配的内存大小

memory_page_t mem_map[ALL_PAGES];
list_head page_inactive = {
    .next = &page_inactive,
    .prev = &page_inactive,
};
static struct spinlock page_inactive_lock;

static void stat_buddy_pages(const char *name, const buddy_t *bud) {
    for (int i = 0; i < BUDDY_ORDER_LIMIT; ++i) {
        const auto area = bud->free_area[i];
        if (area.free_count == 0) { continue; }
        kprintf("%s order=%02d (%02d)", name, i, area.free_count);
        const auto size = block_size[i];
        auto node = area.free_list;
        while (node) {
            const auto pa = pfn_to_phy(page_to_pfn(node));
            kprintf(" 0x%p~0x%p", K_PHY2LIN(pa), K_PHY2LIN(pa + size));
            node = node->next;
        }
        kprintf("\n");
    }
}

void memory_init() {
    kprintf("info: init memory management system\n");

    for (int i = 0; i < BUDDY_ORDER_LIMIT; ++i) { block_size[i] = SZ_4K << i; }

    for (int i = 0; i < ARRAY_SIZE(mem_map); ++i) {
        mem_map[i].next = NULL;
        mem_map[i].inbuddy = false;
    }
    for (size_t i = 0; i < __fmi_ptr()->count; ++i) {
        const size_t first_pfn = phy_to_pfn(__fmi_ptr()->page_ranges[i].base);
        const size_t last_pfn = phy_to_pfn(__fmi_ptr()->page_ranges[i].limit);
        for (size_t j = first_pfn; j < last_pfn; ++j) { mem_map[j].inbuddy = true; }
    }

    //! NOTE: 检测到的可用且有效的物理内存的地址阈限，但并不代表从 0 到此处的物理页都可用
    const size_t detected_phy_mem_limit = __fmi_ptr()->page_ranges[__fmi_ptr()->count - 1].limit;

    big_kernel = detected_phy_mem_limit >= WALL;
    if (big_kernel) {
        kernel_code_size = KKWALL2;
        kernel_size = BigKernelSize;
        buddy_init(kbud, KKWALL2, KUWALL2);
        buddy_init(ubud, KUWALL2, detected_phy_mem_limit);
    } else {
        kernel_code_size = KKWALL1;
        kernel_size = SmallKernelSize;
        buddy_init(kbud, KKWALL1, KUWALL1);
        buddy_init(ubud, KUWALL1, detected_phy_mem_limit);
    }

    stat_buddy_pages("kbud", kbud);
    stat_buddy_pages("ubud", ubud);

    kmem_init();
    init_cache();
}

// 有int型参数size，从内核线性地址空间申请一段大小为size的内存
u32 phy_kmalloc(u32 size) {
    if (size <= (1 << MAX_BUFF_ORDER)) {
        return (u32)kmalloc(size);
    } else if (size == num_4K) {
        return phy_kmalloc_4k();
    }
    return kmalloc_over4k(size);
}

u32 kern_kmalloc(u32 size) {
    return ptr2u(K_PHY2LIN(phy_kmalloc(size)));
}

u32 kern_kzalloc(u32 size) {
    void *ptr = (void *)K_PHY2LIN(phy_kmalloc(size));
    memset(ptr, 0, size);
    return ptr2u(ptr);
}

u32 phy_kfree(u32 phy_addr) {
    assert((!big_kernel && phy_addr < KUWALL1) || (big_kernel && phy_addr < KUWALL2));
    u32 size = get_kmalloc_size(phy_addr);
    if (size <= (1 << MAX_BUFF_ORDER)) {
        return kfree(phy_addr);
    } else {
        return kfree_over4k(phy_addr);
    }
}

// addr must be lin addr
u32 kern_kfree(u32 addr) {
    return phy_kfree(K_LIN2PHY(addr));
}

// modified by mingxuan 2021-8-16
u32 phy_kmalloc_4k() // 无参数，从内核线性地址空间申请一页内存
{
    memory_page_t *page = alloc_pages(kbud, 0);
    int res = pfn_to_phy(page_to_pfn(page));
    if (page == 0) kprintf("phy_kmalloc_4k: alloc_pages Error,no memory");
    return res;
}

// added by mingxuan 2021-8-17
u32 kern_kmalloc_4k() {
    return ptr2u(K_PHY2LIN(phy_kmalloc_4k()));
}

// modified by mingxuan 2021-8-16
u32 phy_kfree_4k(u32 phy_addr) // 有unsigned
                               //  int型参数addr，释放掉起始地址为addr的一段内存，大小由内存管理决定
{
    if (phy_addr % num_4K) {
        kprintf("phy_free_4k: addr Error");
        return 0;
    }

    if (big_kernel == 1) {
        if (phy_addr >= KUWALL2) {
            kprintf("phy_kfree_4k: addr Error");
            return 0;
        }
    } else {
        if (phy_addr >= KUWALL1) {
            kprintf("phy_kfree_4k: addr Error");
            return 0;
        }
    }

    memory_page_t *page = pfn_to_page(phy_to_pfn(phy_addr));
    return free_pages(kbud, page, 0);
}

// added by mingxuan 2021-8-17
u32 kern_kfree_4k(u32 addr) // this addr must be lin addr
{
    return (phy_kfree_4k(K_LIN2PHY(addr)));
}

// modified by mingxuan 2021-8-14
u32 phy_malloc_4k() // 无参数，从用户线性地址空间堆中申请一页内存
{
    memory_page_t *page = alloc_pages(ubud, 0);
    int res = pfn_to_phy(page_to_pfn(page));
    if (page == 0) kprintf("phy_malloc_4k:alloc_pages Error,no memory");
    return res;
}

// phy_free_4k, the param addr mush be phy addr, mingxuan 2021-8-16
u32 phy_free_4k(u32 phy_addr) // 有unsigned
                              //  int型参数addr，释放掉起始地址为addr的一段内存，大小由内存管理决定
{
    if (phy_addr % num_4K != 0) {
        kprintf("phy_free_4k: addr Error1");
        return 0;
    }

    if (big_kernel == 1) {
        if (phy_addr <= KUWALL2) {
            kprintf("phy_free_4k: addr Error2");
            return 0;
        }
    } else {
        if (phy_addr <= KUWALL1) {
            kprintf("phy_free_4k: addr Error3");
            return 0;
        }
    }
    memory_page_t *page = pfn_to_page(phy_to_pfn(phy_addr));
    if (atomic_dec_and_test(&page->count)) { return free_pages(ubud, page, 0); }
    return 0;
}

u32 kern_malloc_4k() // modified by mingxuan 2021-8-19
{
    u32 AddrLin;

    AddrLin = get_heap_limit(p_proc_current->task.pid);
    update_heap_limit(p_proc_current->task.pid, 1);
    // kern_mapping_4k(AddrLin, p_proc_current->task.pid,
    // 	MAX_UNSIGNED_INT, PG_P | PG_USU | PG_RWW);
    int addr = do_mmap(AddrLin, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
    if (addr == -1) {
        kprintf("kern_malloc_4k: error mmap addr occupied");
        return 0x00000000;
    }
    return AddrLin;
}

// added by mingxuan 2021-8-19
u32 do_malloc_4k() {
    return kern_malloc_4k();
}

// added by mingxuan 2021-8-11
u32 sys_malloc_4k() {
    return do_malloc_4k();
}

// syscall free
//
int kern_free_4k(void *AddrLin) // modified by mingxuan 2021-8-19
{
    // int phy_addr;

    // phy_addr = get_page_phy_addr(p_proc_current->task.pid, (int)AddrLin);
    // //获取物理页的物理地址

    // clear_pte(p_proc_current->task.pid, (u32)AddrLin);

    // update_heap_ptr(AddrLin,-1); //update heap pointer
    update_heap_limit(p_proc_current->task.pid,
                      -1); // update heap limit edited by wang 2021.8.26

    // return phy_free_4k(phy_addr);
    return do_munmap((u32)AddrLin, PAGE_SIZE);
}

int do_free_4k(void *AddrLin) {
    return kern_free_4k(AddrLin);
}

int sys_free_4k() {
    return do_free_4k((void *)get_arg(1));
}

// mmu for user vmem addr

void get_page(memory_page_t *_page) {
    if (atomic_get(&_page->count) == 0) {
        acquire(&page_inactive_lock);
        list_remove(&_page->pg_lru);
        release(&page_inactive_lock);
    }
    atomic_inc(&_page->count);
}

static int drop_page(memory_page_t *_page) {
    int ret = 0;
    struct address_space *mapping = _page->pg_mapping;
    if (mapping) {
        lock_or_yield(&mapping->lock);
        ret = free_mem_page(_page);
        release(&mapping->lock);
    } else {
        ret = free_pages(ubud, _page, 0);
    }
    return ret;
}

static memory_page_t *get_and_remove_inactive_page() {
    acquire(&page_inactive_lock);
    memory_page_t *_page = list_back(&page_inactive, memory_page_t, pg_lru);
    list_remove(&_page->pg_lru);
    release(&page_inactive_lock);
    return _page;
}

void shrink_page_memory() {
    memory_page_t *_page;
    int nr_shrink = 0, max_shrink = 16;
    // 因为上锁顺序的限制，持有page_inactive_lock不能再获取mapping的锁(防死锁)
    while (nr_shrink < max_shrink && (_page = get_and_remove_inactive_page())) {
        drop_page(_page);
        nr_shrink++;
    }
}

int put_page(memory_page_t *_page) {
    if (atomic_dec_and_test(&_page->count)) {
        int type = MEMPAGE_AUTO, ret = 0;
        if (_page->pg_mapping) { type = _page->pg_mapping->type; }
        switch (type) {
            case MEMPAGE_AUTO: // 此页面是进程私有数据，不再使用直接释放
                ret = drop_page(_page);
                break;
            case MEMPAGE_CACHED: // 通常是文件映射的页面，将_page插入RLU,
                                 // 此页面暂时不再使用，可能在未来内存不足时释放
                acquire(&page_inactive_lock);
                list_add_first(&_page->pg_lru, &page_inactive);
                release(&page_inactive_lock);
            case MEMPAGE_SAVE: // 此页面暂时不再使用，由其他组件手动释放
            default:
                break;
        }
        return ret;
    }
    return 0;
}

// find first vma->end > addr
// require: pcb.memmap.vma_lock
struct vmem_area *find_vma(LIN_MEMMAP *mmap, u32 addr) {
    struct vmem_area *vma = NULL;
    list_for_each(&mmap->vma_map, vma, vma_list) {
        if (vma->end > addr) { return vma; }
    }
    return NULL;
}

memory_page_t *alloc_user_page(u32 pgoff) {
    if (ubud->current_mem_size < (ubud->total_mem_size >> 4)) { shrink_page_memory(); }
    memory_page_t *_page = alloc_pages(ubud, 0);
    atomic_set(&_page->count, 1);
    _page->pg_off = pgoff;
    _page->pg_mapping = NULL;
    list_init(&_page->pg_list);
    list_init(&_page->pg_lru);
    memset(_page->pg_buffer, 0, sizeof(_page->pg_buffer));
    kmap(_page);
    return _page;
}

// req. ring 0
static void copy_page(memory_page_t *dst, memory_page_t *src) {
    u32 dst_vaddr = ptr2u(kpage_lin(dst));
    u32 src_vaddr = ptr2u(kpage_lin(src));
    memcpy((void *)dst_vaddr, (void *)src_vaddr, PAGE_SIZE);
}

// req. ring 0
void zero_page(memory_page_t *_page) {
    u32 dst_vaddr = ptr2u(kpage_lin(_page));
    // dst_vaddr = kmap(_page);
    memset((void *)dst_vaddr, 0, PAGE_SIZE);
}

void copy_from_page(memory_page_t *_page, void *buf, u32 len, u32 offset) {
    // u32 vaddr = kmap(_page);
    u32 vaddr = ptr2u(kpage_lin(_page));
    memcpy(buf, (void *)(vaddr + offset), len);
    // kunmap(_page);
}

void copy_to_page(memory_page_t *_page, const void *buf, u32 len, u32 offset) {
    // u32 vaddr = kmap(_page);
    u32 vaddr = ptr2u(kpage_lin(_page));
    memcpy((void *)vaddr + offset, buf, len);
    // kunmap(_page);
}

static struct address_space *get_mapping(LIN_MEMMAP *mmap, struct vmem_area *vma) {
    struct address_space *mapping = NULL;
    if (vma->file) { // file mapped memory MEMPAGE_CACHED
        mapping = vma->file->fd_mapping;
    } else if (vma->flags & MAP_PRIVATE) { // process private allocated memory MEMPAGE_AUTO
        mapping = &mmap->anon_pages;
    } else {
        mapping = &shm_pages; // shared mem managed by hand MEMPAGE_SAVE
    }
    return mapping;
}

static memory_page_t *find_or_create_page(LIN_MEMMAP *mmap, struct vmem_area *vma, u32 pgoff) {
    struct address_space *mapping = get_mapping(mmap, vma);
    lock_or_yield(&mapping->lock);
    memory_page_t *_page = find_mem_page(mapping, pgoff);
    if (_page == NULL) {
        _page = alloc_user_page(pgoff);
        if (vma->file) {
            lock_or_yield(&mapping->host->lock);
            generic_file_readpage(mapping, _page);
            release(&mapping->host->lock);
        } else {
            zero_page(_page);
        }
        add_mem_page(mapping, _page);
    } else {
        get_page(_page);
    }
    release(&mapping->lock);
    return _page;
}

static void free_vma_pages(PROCESS *p_proc, LIN_MEMMAP *mmap, struct vmem_area *vma) {
    UNUSED(mmap);
    u32 nr_pages = (vma->end - vma->start) >> PAGE_SHIFT;
    u32 addr = vma->start;
    for (u32 i = 0; i < nr_pages; i++) {
        u32 phy = get_page_phy_addr(proc2pid(p_proc), addr);
        if (phy) {
            memory_page_t *_page = pfn_to_page(phy_to_pfn(phy));
            put_page(_page);
            clear_pte(proc2pid(p_proc), addr);
        }
        addr += PAGE_SIZE;
    }
}

// 直接为vma申请内存并设置页表, 没有写时复制
// require: pcb.memmap.vma_lock
void prepare_vma(PROCESS *p_proc, LIN_MEMMAP *mmap, struct vmem_area *vma) {
    memory_page_t *_page = NULL;
    u32 pgoff = vma->pgoff;
    // 页面权限
    u32 pte_attr = PG_P | PG_USU | ((vma->flags & PROT_WRITE) ? PG_RWW : PG_RWR);
    for (u32 addr = vma->start; addr < vma->end; addr += PAGE_SIZE, pgoff++) {
        memory_page_t *content_page = find_or_create_page(mmap, vma, pgoff);
        // 已经增加物理页的引用计数
        // 对于可写的私有文件映射，不能修改文件，所以做一份复制
        if ((vma->file) && (vma->flags & PROT_WRITE) && (vma->flags & MAP_PRIVATE)) {
            _page = alloc_user_page(addr >> PAGE_SHIFT);
            copy_page(_page, content_page);
            put_page(content_page);
            add_mem_page(&mmap->anon_pages, _page);
        } else {
            // 其他情况直接使用page cache的物理页就行
            _page = content_page;
        }
        map_laddr(addr, pfn_to_phy(page_to_pfn(_page)), proc2pid(p_proc), PG_P | PG_USU | PG_RWW,
                  pte_attr);
    }
}

static void free_vma(PROCESS *p_proc, LIN_MEMMAP *mmap, struct vmem_area *vma) {
    list_remove(&vma->vma_list);
    // clear arch pgtable and free
    free_vma_pages(p_proc, mmap, vma);
    if (vma->file) { fput(vma->file); }
    kern_kfree((u32)vma);
}

// free vma [start, last)
void free_vmas(PROCESS *p_proc, LIN_MEMMAP *mmap, struct vmem_area *start, struct vmem_area *last) {
    struct vmem_area *vma = start, *next;
    while (vma && vma != last) {
        next = list_next(&mmap->vma_map, vma, vma_list);
        free_vma(p_proc, mmap, vma);
        vma = next;
    }
}

/// @brief 复制进程的内存映射(fork 内部调用)
/// @param p_parent 父进程
/// @param p_child 子进程
/// @details
/// 当 写时复制(OPT_MMU_COW)启用:
/// 	fork时，将父进程的所有有效页表项设置为对应虚拟地址的只读页表项，并增加对应物理页的引用计数
/// 	fork之后，只读的页面父子进程使用同一个物理页，可写的页面在写操作时发生缺页，
/// 	由处理程序复制物理页内容到新匿名页面完成COW处理
/// 写时复制(OPT_MMU_COW)关闭时:
/// 	fork的过程中不改变父进程的页表，根据权限判断引用父进程的物理页或者复制父进程的物理页，并设置好子进程的页表项
void memmap_copy(PROCESS *p_parent, PROCESS *p_child) {
    LIN_MEMMAP *old_mmap = proc_memmap(p_parent);
    LIN_MEMMAP *new_mmap = proc_memmap(p_child);
    *new_mmap = *old_mmap;
    init_mem_page(&new_mmap->anon_pages, MEMPAGE_AUTO);
    list_init(&new_mmap->vma_map);
    struct vmem_area *vma, *vm;
    lock_or_yield(&old_mmap->vma_lock);
    list_for_each(&old_mmap->vma_map, vma, vma_list) {
        vm = (struct vmem_area *)kern_kmalloc(sizeof(struct vmem_area));
        *vm = *vma;
        list_init(&vm->vma_list);
        if (vm->file) { fget(vm->file); }
        list_add_last(&vm->vma_list, &new_mmap->vma_map);
        u32 pte_attr = PG_P | PG_USU | ((vma->flags & PROT_WRITE) ? PG_RWW : PG_RWR);
        for (u32 addr = vma->start; addr < vma->end; addr += PAGE_SIZE) {
            u32 phy = get_page_phy_addr(proc2pid(p_parent), addr);
            if (phy) {
                memory_page_t *pte_page = pfn_to_page(phy_to_pfn(phy));
#ifdef OPT_MMU_COW
                // 写时复制，将父子进程页表同时置为只读
                get_page(pte_page); // add reference
                lin_mapping_phy(addr, phy, proc2pid(p_parent), PG_P | PG_USU | PG_RWW,
                                PG_P | PG_USU | PG_RWR); // set read only
                lin_mapping_phy(addr, phy, proc2pid(p_child), PG_P | PG_USU | PG_RWW,
                                PG_P | PG_USU | PG_RWR);
#else
                // 无写时复制，
                // 根据权限决定，只读的页面共享，可写的私有页面申请新页面并复制
                memory_page_t *_page = NULL;
                if ((vma->flags & MAP_PRIVATE) && (vma->flags & PROT_WRITE)) {
                    _page = alloc_user_page(addr >> PAGE_SHIFT);
                    copy_page(_page, pte_page);
                    add_mem_page(&new_mmap->anon_pages, _page);
                } else {
                    // 其他情况直接使用父进程的物理页就行,要增加父进程物理页的引用计数
                    get_page(pte_page);
                    _page = pte_page;
                }
                map_laddr(addr, pfn_to_phy(page_to_pfn(_page)), proc2pid(p_child),
                          PG_P | PG_USU | PG_RWW, pte_attr);
#endif
            }
        }
    }
    release(&old_mmap->vma_lock);
}

void memmap_clear(PROCESS *p_proc) {
    LIN_MEMMAP *mmap = proc_memmap(p_proc);
    free_vmas(p_proc, mmap, list_front(&mmap->vma_map, struct vmem_area, vma_list), NULL);
}

// handle generic mm fault, return 0 if handle normal ok, -1 for error
int handle_mm_fault(LIN_MEMMAP *mmap, u32 vaddr, int flag) {
#ifndef OPT_MMU_COW
    kprintf("error: no cow, this handler is unreachable!");
#endif
    struct vmem_area *vma = find_vma(mmap, vaddr);
    if (vma && vaddr >= vma->start) {
        u32 pgoff = vma->pgoff + ((vaddr - vma->start) >> PAGE_SHIFT);
        memory_page_t *_page = NULL;
        u32 attr = PG_P | PG_USU; // 这里目前没有做架构分离
        u32 phy = get_page_phy_addr(proc2pid(p_proc_current), vaddr);
        memory_page_t *pte_page = NULL;
        if ((void *)phy != NULL) { pte_page = pfn_to_page(phy_to_pfn(phy)); }
        if (flag & FAULT_NOPAGE) { // handler should check page and set pte
            if (pte_page == NULL) {
                // 页表完全没有设置物理页，需要找到正确的物理页
                _page = find_or_create_page(mmap, vma, pgoff);
                if (vma->file && vma->flags & MAP_PRIVATE) {
                    attr |= PG_RWR;
                } else {
                    attr |= ((vma->flags & PROT_WRITE) ? PG_RWW : PG_RWR);
                }
            } else { // pte 存在物理地址，但是页面不存在，所以页面被换出了
                // 由于暂时不存在相应机制，暂不处理
                // 如果运行到此处, 说明页表存在问题
                kprintf("error: page swapped? or just wrong pte?");
            }
        } else if ((flag & FAULT_WRITE) && (vma->flags & PROT_WRITE)) {
            if (pte_page == NULL) { kprintf("error: no page for page present fault"); } // error
            if (vma->flags & MAP_PRIVATE) { // file private RW: COW now
                _page = alloc_user_page(vaddr >> PAGE_SHIFT);
                copy_page(_page, pte_page);
                put_page(pte_page);
                add_mem_page(&mmap->anon_pages, _page);
                attr |= PG_RWW;
            }
        }

        if (_page) { // now new page is ready
            map_laddr(vaddr, pfn_to_phy(page_to_pfn(_page)), proc2pid(p_proc_current),
                      PG_P | PG_USU | PG_RWW, attr);
            return 0;
        }
    }
    return -1; // invalid addr;
}

u32 kern_total_mem_size() {
    u32 total_mem_size = 0;
    total_mem_size = kbud->current_mem_size + ubud->current_mem_size + kmem.current_mem_size;
    return total_mem_size;
}
