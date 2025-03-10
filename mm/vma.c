#include <minios/memman.h>
#include <minios/page.h>
#include <minios/mmap.h>
#include <minios/vfs.h>
#include <minios/shm.h>
#include <minios/assert.h>
#include <minios/console.h>
#include <minios/kstate.h>
#include <fs/fs.h>

static address_space_t *get_mapping(memmap_t *mmap, struct vmem_area *vma) {
    address_space_t *mapping = NULL;
    if (vma->file) { // file mapped memory MEMPAGE_CACHED
        mapping = vma->file->fd_mapping;
    } else if (vma->flags & MAP_PRIVATE) { // process private allocated memory MEMPAGE_AUTO
        mapping = &mmap->anon_pages;
    } else {
        mapping = &shm_pages; // shared mem managed by hand MEMPAGE_SAVE
    }
    return mapping;
}

static memory_page_t *find_or_create_page(memmap_t *mmap, struct vmem_area *vma, u32 pgoff) {
    address_space_t *mapping = get_mapping(mmap, vma);
    spinlock_lock_or_yield(&mapping->lock);
    memory_page_t *page = find_mem_page(mapping, pgoff);
    if (page != NULL) {
        get_page(page);
    } else {
        page = alloc_user_page(pgoff);
        assert(page != NULL);
        if (vma->file) {
            spinlock_lock_or_yield(&mapping->host->lock);
            generic_file_readpage(mapping, page);
            spinlock_release(&mapping->host->lock);
        } else if (!kstate_on_init) {
            //! FIXME: write to user pages in kerel setup stage is not allowed but we might need it
            zero_page(page);
        }
        add_mem_page(mapping, page);
    }
    spinlock_release(&mapping->lock);
    return page;
}

static void free_vma_pages(process_t *p_proc, memmap_t *mmap, struct vmem_area *vma) {
    UNUSED(mmap);
    u32 nr_pages = phy_to_pfn(vma->end - vma->start);
    u32 addr = vma->start;
    for (u32 i = 0; i < nr_pages; ++i) {
        u32 phy = get_page_phy_addr(proc2pid(p_proc), addr);
        if (phy) {
            memory_page_t *_page = pfn_to_page(phy_to_pfn(phy));
            put_page(_page);
            clear_pte(proc2pid(p_proc), addr);
        }
        addr += PGSIZE;
    }
}

// 直接为vma申请内存并设置页表, 没有写时复制
// require: pcb.memmap.vma_lock
void prepare_vma(process_t *p_proc, memmap_t *mmap, struct vmem_area *vma) {
    memory_page_t *_page = NULL;
    u32 pgoff = vma->pgoff;
    // 页面权限
    u32 pte_attr = PG_P | PG_USU | ((vma->flags & PROT_WRITE) ? PG_RWW : PG_RWR);
    for (u32 addr = vma->start; addr < vma->end; addr += PGSIZE, pgoff++) {
        memory_page_t *content_page = find_or_create_page(mmap, vma, pgoff);
        // 已经增加物理页的引用计数
        // 对于可写的私有文件映射，不能修改文件，所以做一份复制
        if ((vma->file) && (vma->flags & PROT_WRITE) && (vma->flags & MAP_PRIVATE)) {
            _page = alloc_user_page(phy_to_pfn(addr));
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

static void free_vma(process_t *p_proc, memmap_t *mmap, struct vmem_area *vma) {
    list_remove(&vma->vma_list);
    // clear arch pgtable and free
    free_vma_pages(p_proc, mmap, vma);
    if (vma->file) { fput(vma->file); }
    kern_kfree(vma);
}

// free vma [start, last)
void free_vmas(process_t *p_proc, memmap_t *mmap, struct vmem_area *start, struct vmem_area *last) {
    struct vmem_area *vma = start;
    while (vma && vma != last) {
        struct vmem_area *next = list_next(&mmap->vma_map, vma, vma_list);
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
void memmap_copy(process_t *p_parent, process_t *p_child) {
    memmap_t *old_mmap = proc_memmap(p_parent);
    memmap_t *new_mmap = proc_memmap(p_child);
    *new_mmap = *old_mmap;
    init_mem_page(&new_mmap->anon_pages, MEMPAGE_AUTO);
    list_init(&new_mmap->vma_map);
    struct vmem_area *vma, *vm;
    spinlock_lock_or_yield(&old_mmap->vma_lock);
    list_for_each(&old_mmap->vma_map, vma, vma_list) {
        vm = kern_kmalloc(sizeof(struct vmem_area));
        *vm = *vma;
        list_init(&vm->vma_list);
        if (vm->file) { fget(vm->file); }
        list_add_last(&vm->vma_list, &new_mmap->vma_map);
        u32 pte_attr = PG_P | PG_USU | ((vma->flags & PROT_WRITE) ? PG_RWW : PG_RWR);
        for (u32 addr = vma->start; addr < vma->end; addr += PGSIZE) {
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
                    _page = alloc_user_page(phy_to_pfn(addr));
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
    spinlock_release(&old_mmap->vma_lock);
}

void memmap_clear(process_t *p_proc) {
    memmap_t *mmap = proc_memmap(p_proc);
    free_vmas(p_proc, mmap, list_front(&mmap->vma_map, struct vmem_area, vma_list), NULL);
}

int handle_mm_fault(memmap_t *mmap, u32 vaddr, int flag) {
#ifndef OPT_MMU_COW
    kprintf("error: no cow, this handler is unreachable!");
#endif
    struct vmem_area *vma = find_vma(mmap, vaddr);
    if (vma && vaddr >= vma->start) {
        u32 pgoff = vma->pgoff + phy_to_pfn(vaddr - vma->start);
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
                _page = alloc_user_page(phy_to_pfn(vaddr));
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
