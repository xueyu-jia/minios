#include "memman.h"
#include "buddy.h" //added by mingxuan 2021-3-8
#include "slab.h"
#include "pagetable.h"
#include "shm.h"
#include "fs.h"
#include "proto.h"
#include "mmap.h"
#include "pagecache.h"
#include "kmalloc.h"
#include "string.h"

int big_kernel = 0;        //当big_kernel=1时，表示大内核，big_kernel=0表示小内核，added by wang 2021.8.16
u32 kernel_size = 0;       //表示内核大小的全局变量，added by wang 2021.8.27
u32 kernel_code_size = 0;  //为内核代码数据分配的内存大小，     added by wang 2021.8.27
u32 test_phy_mem_size = 0; //检测到的物理机的物理内存的大小，    added by wang 2021.8.27
u32 MemInfo[256] = {0}; //存放FMIBuff后1k内容
page mem_map[ALL_PAGES];
list_head page_inactive_lru = {
	.next = &page_inactive_lru,
	.prev = &page_inactive_lru,
};
PRIVATE struct spinlock page_inactive_lock;

void memory_init()
{
    memcpy(MemInfo, (u32 *)FMIBuff, 1024); //复制内存

    test_phy_mem_size = MemInfo[MemInfo[0]];
    // int t, i, j;
	u32 i;
    // disp_str("memtest got memory available:\n");
    // disp_str("base_addr     end_addr\n");
    // for (t = 1; t <= MemInfo[0]; t++)
    // {
    //     disp_int(MemInfo[t]);
    //     disp_str("       ");
    //     if (t % 2 == 0)
    //         disp_str("\n");
    // }
    disp_str("test_phy_mem_size: ");
    disp_int(test_phy_mem_size);
    disp_str("\n");

    u32 bsize = num_4K;
    for (i = 0; i < MAX_ORDER; i++) {
        block_size[i] = bsize;
        bsize = 2 * bsize;
    }

    // TODO: ALL_PAGES 怎么确定
    u32 last_pfn = phy_to_pfn(test_phy_mem_size);
    for (i = 0; i < last_pfn; i++) {
        mem_map[i].inbuddy = TRUE;
    }
    for (; i < ALL_PAGES; i++) {
        mem_map[i].inbuddy = FALSE;
    }

    if (test_phy_mem_size < WALL) {
        big_kernel = 0;
        kernel_code_size = KKWALL1;
        kernel_size = SmallKernelSize;
        buddy_init(kbud, KKWALL1, KUWALL1);
        buddy_init(ubud, KUWALL1, test_phy_mem_size);
    }
    else {
        big_kernel = 1;
        kernel_code_size = KKWALL2;
        kernel_size = BigKernelSize;
        buddy_init(kbud, KKWALL2, KUWALL2);
        buddy_init(ubud, KUWALL2, test_phy_mem_size);
    }

    kmem_init(); //added by mingxuan 2021-3-8
    init_cache();
}

//modified by mingxuan 2021-8-16
PUBLIC u32 phy_kmalloc(u32 size) //有int型参数size，从内核线性地址空间申请一段大小为size的内存
{
	if (size <= (1 << MAX_BUFF_ORDER)) {
		return (u32)kmalloc(size);
	} else if(size == num_4K) {
		return phy_kmalloc_4k();
	}
	return kmalloc_over4k(size);
}

//added by mingxuan 2021-8-16
PUBLIC u32 kern_kmalloc(u32 size)
{
	return K_PHY2LIN(phy_kmalloc(size));
}
//add by sundong 2023.6.3
//分配一段内存,并初始化为0
PUBLIC u32 kern_kzalloc(u32 size)
{
	void *p = (void*)K_PHY2LIN(phy_kmalloc(size));
	memset(p,0,size);
	return (u32)p;
}

//在内核中直接使用,mingxuan 2021-3-25
//edited by wang 2021.6.8
/*
PUBLIC u32 do_kfree(u32 addr) //有unsigned int型参数addr和size，释放掉起始地址为addr长度为size的一段内存
{
	if (big_kernel == 1)
	{
		if (addr >= KUWALL2)
		{
			disp_color_str("do_kfree_4k: addr Error", 0x74);
			return 0;
		}
	}
	else
	{
		if (addr >= KUWALL1)
		{
			disp_color_str("do_kfree_4k: addr Error", 0x74);
			return 0;
		}
	}

	u32 size = get_kmalloc_size(addr);
	if (size < num_4K)
		return kfree(addr);
	else
		return kfree_over4k(addr);
}
*/
PUBLIC u32 phy_kfree(u32 phy_addr) //有unsigned int型参数addr和size，释放掉起始地址为addr长度为size的一段内存
{
	if (big_kernel == 1)
	{
		if (phy_addr >= KUWALL2)
		{
			disp_color_str("phy_kfree: addr Error", 0x74);
			return 0;
		}
	}
	else
	{
		if (phy_addr >= KUWALL1)
		{
			disp_color_str("phy_kfree: addr Error", 0x74);
			return 0;
		}
	}

	u32 size = get_kmalloc_size(phy_addr);
	if (size <= (1 << MAX_BUFF_ORDER))
		return kfree(phy_addr);
	else
		return kfree_over4k(phy_addr);
}

//added by mingxuan 2021-8-17
PUBLIC u32 kern_kfree(u32 addr) //addr must be lin addr
{
	return phy_kfree(K_LIN2PHY(addr));
}

//在内核中直接使用,mingxuan 2021-3-25
/* //deleted by mingxuan 2021-8-16
PUBLIC u32 do_kmalloc_4k() //无参数，从内核线性地址空间申请一页内存
{
	int res = alloc_pages(kbud, 0);
	if (res == 0)
		disp_color_str("do_kmalloc_4k:alloc_pages Error,no memory", 0x74);
	return res;
}
*/

//modified by mingxuan 2021-8-16
PUBLIC u32 phy_kmalloc_4k() //无参数，从内核线性地址空间申请一页内存
{
	page *page = alloc_pages(kbud, 0);
	atomic_set(&page->count, 1);
    int res = pfn_to_phy(page_to_pfn(page));
	// disp_str(" ka");
	// disp_int(res);
	if (res == 0)
		disp_color_str("phy_kmalloc_4k: alloc_pages Error,no memory", 0x74);
	return res;
}

//added by mingxuan 2021-8-17
PUBLIC u32 kern_kmalloc_4k()
{
	return K_PHY2LIN(phy_kmalloc_4k());
}

//在内核中直接使用,mingxuan 2021-3-25
/*
PUBLIC u32 do_kfree_4k(u32 addr) //有unsigned int型参数addr，释放掉起始地址为addr的一段内存，大小由内存管理决定
{
	if (big_kernel == 1)
	{
		if (addr >= KUWALL2)
		{
			disp_color_str("do_kfree_4k: addr Error", 0x74);
			return 0;
		}
	}
	else
	{
		if (addr >= KUWALL1)
		{
			disp_color_str("do_kfree_4k: addr Error", 0x74);
			return 0;
		}
	}

	return free_pages(kbud, addr, 0);
}
*/

PUBLIC void get_page(page *_page) {
	if(atomic_get(&_page->count) == 0) {
		acquire(&page_inactive_lock);
		list_remove(&_page->pg_lru);
		release(&page_inactive_lock);
	}
	atomic_inc(&_page->count);
	// disp_str("get:");
	// disp_int(page_to_pfn(_page));
	// disp_str(" ");
}

PUBLIC int put_page(page *_page, int cache) {
	// disp_str("put:");
	// disp_int(page_to_pfn(_page));
	// disp_str(" ");
	if (atomic_dec_and_test(&_page->count)) {
		if(cache) { // move to inactive
			acquire(&page_inactive_lock);
			list_add_last(&_page->pg_lru, &page_inactive_lru);
			release(&page_inactive_lock);
			return 0;
		}
		acquire(&_page->pg_cache->lock);
		list_remove(&_page->pg_list);
		release(&_page->pg_cache->lock);
		return free_pages(ubud, _page, 0);
	}
	return 0;
}

//modified by mingxuan 2021-8-16
PUBLIC u32 phy_kfree_4k(u32 phy_addr) //有unsigned int型参数addr，释放掉起始地址为addr的一段内存，大小由内存管理决定
{
	if (phy_addr % num_4K)
	{
		disp_color_str("phy_free_4k: addr Error", 0x74);
		return 0;
	}

	if (big_kernel == 1)
	{
		if (phy_addr >= KUWALL2)
		{
			disp_color_str("phy_kfree_4k: addr Error", 0x74);
			return 0;
		}
	}
	else
	{
		if (phy_addr >= KUWALL1)
		{
			disp_color_str("phy_kfree_4k: addr Error", 0x74);
			return 0;
		}
	}

	// disp_str(" kf");
	// disp_int(phy_addr);
    page *page = pfn_to_page(phy_to_pfn(phy_addr));
	if (atomic_dec_and_test(&page->count)) {
		return free_pages(kbud, page, 0);
	}
	return 0;
}

//added by mingxuan 2021-8-17
PUBLIC u32 kern_kfree_4k(u32 addr) //this addr must be lin addr
{
	return (phy_kfree_4k(K_LIN2PHY(addr)));
}

//在用户态下通过malloc调用sys_malloc_4k,进而使用。不可以在内核中使用。mingxuan 2021-3-25
/*
PUBLIC u32 do_malloc_4k() //无参数，从用户线性地址空间堆中申请一页内存
{
	int res = alloc_pages(ubud, 0);
	if (res == 0)
		disp_color_str("do_malloc_4k:alloc_pages Error,no memory", 0x74);
	return res;
}
*/
//modified by mingxuan 2021-8-14
PUBLIC u32 phy_malloc_4k() //无参数，从用户线性地址空间堆中申请一页内存
{
	page *page = alloc_pages(ubud, 0);
	atomic_set(&page->count, 1);
    int res = pfn_to_phy(page_to_pfn(page));
	// disp_str(" a");
	// disp_int(res);
	if (res == 0)
		disp_color_str("phy_malloc_4k:alloc_pages Error,no memory", 0x74);
	return res;
}


// PUBLIC void phy_mapping_4k(u32 phy_addr) {
// 	// disp_str(" m");
// 	// disp_int(phy_addr);
// 	page *page = pfn_to_page(phy_to_pfn(phy_addr));
// 	atomic_inc(&page->count);
// }
//added by mingxuan 2021-8-16
/* //deleted by mingxuan 2021-8-19, moved to syscallc.c
PUBLIC u32 kern_malloc_4k()
{
	return K_PHY2LIN(phy_malloc_4k());
}
*/

//在用户态下通过free调用sys_free_4k,进而使用。不可以在内核中使用。mingxuan 2021-3-25
/*
PUBLIC u32 do_free_4k(u32 addr) //有unsigned int型参数addr，释放掉起始地址为addr的一段内存，大小由内存管理决定
{
	if (big_kernel == 1)
	{
		if (addr <= KUWALL2)
		{
			disp_color_str("do_free_4k: addr Error", 0x74);
			return 0;
		}
	}
	else
	{
		if (addr <= KUWALL1)
		{
			disp_color_str("do_free_4k: addr Error", 0x74);
			return 0;
		}
	}
	return free_pages(ubud, addr, 0);
}
*/

//modified by mingxuan 2021-8-14
//phy_free_4k, the param addr mush be phy addr, mingxuan 2021-8-16
PUBLIC u32 phy_free_4k(u32 phy_addr) //有unsigned int型参数addr，释放掉起始地址为addr的一段内存，大小由内存管理决定
{

	if (phy_addr % num_4K != 0)
	{
		disp_color_str("phy_free_4k: addr Error1", 0x74);
		return 0;
	}

	if (big_kernel == 1)
	{
		if (phy_addr <= KUWALL2)
		{
			disp_color_str("phy_free_4k: addr Error2", 0x74);
			return 0;
		}
	}
	else
	{
		if (phy_addr <= KUWALL1)
		{
			disp_color_str("phy_free_4k: addr Error3", 0x74);
			return 0;
		}
	}
	// disp_str(" f");
	// disp_int(phy_addr);
	page *page = pfn_to_page(phy_to_pfn(phy_addr));
	if (atomic_dec_and_test(&page->count)) {
		return free_pages(ubud, page, 0);
	}
	return 0;
}

//added by mingxuan 2021-8-16
//kern_free_4k, the param addr mush be lin addr, mingxuan 2021-8-16
/*
PUBLIC u32 kern_free_4k(u32 addr) //addr is lin address
{
	return phy_free_4k(K_LIN2PHY(addr));
}
*/

//此函数已废弃不用 //deleted by mingxuan 2021-3-25
/*
PUBLIC u32 do_free(u32 addr,u32 size) //有unsigned int型参数addr和size，释放掉起始地址为addr长度为size的一段内存
{
        disp_str("\ndo_free: ");
        disp_int(addr);
        disp_str("  ");
        return 0;
}
*/

/*
//added by wang 2021.6.8
PUBLIC u32 do_kmalloc_over4k(u32 size) //无参数，从内核线性地址空间申请一页内存
{
    return kmalloc_over4k(size);
}




//added by wang 2021.6.8
PUBLIC u32 do_kfree_over4k(u32 addr)  //有unsigned int型参数addr，释放掉起始地址为addr的一段内存，大小由内存管理决定
{

        return kfree_over4k(addr);

}

*/
// PUBLIC int kern_mapping_4k(u32 AddrLin, u32 pid, u32 phyAddr, u32 pte_attribute) {
// 	if (phyAddr != MAX_UNSIGNED_INT) {
// 		phy_mapping_4k(phyAddr);
// 	}
// 	return lin_mapping_phy(AddrLin, phyAddr, pid, PG_P | PG_USU | PG_RWW, pte_attribute);
// }

// PUBLIC int ker_umalloc_4k(u32 AddrLin, u32 pid, u32 pte_attribute)
// {

// 	//不用判AddrLin是否4K对齐，lin_mapping_phy会为非4k对齐的线性地址分配物理页

// 	if (AddrLin > KernelLinBase)
// 	{
// 		disp_color_str("ker_umalloc_4k: address error ", 0x74);
// 		return -1;
// 	}

// 	return kern_mapping_4k(AddrLin, pid, MAX_UNSIGNED_INT, pte_attribute);
// }

//added by wang 2021.8.24

// PUBLIC int ker_ufree_4k(u32 pid, u32 AddrLin)
// {
// 	u32 phy_addr = 0;

// 	if (AddrLin > KernelLinBase)
// 	{

// 		disp_color_str("ker_ufree_4k: address error ", 0x74);
// 		return -1;
// 	}
// 	else //释放物理页
// 	{
// 		phy_addr = get_page_phy_addr(pid, AddrLin);
// 		if(phy_addr)
// 		phy_free_4k(phy_addr);
// 		clear_pte(pid, AddrLin);
// 		return 0;
// 	}
// }

PUBLIC u32 kern_malloc_4k() //modified by mingxuan 2021-8-19
{

	u32 AddrLin;

	AddrLin = get_heap_limit(p_proc_current->task.pid);
	update_heap_limit(p_proc_current->task.pid, 1);
	// kern_mapping_4k(AddrLin, p_proc_current->task.pid, 
	// 	MAX_UNSIGNED_INT, PG_P | PG_USU | PG_RWW);
	int addr = do_mmap(AddrLin, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE, -1, 0);
	if(addr == -1) {
		disp_str("kern_malloc_4k: error mmap addr occupied");
		return NULL;
	}
	return AddrLin;
}

//added by mingxuan 2021-8-19
PUBLIC u32 do_malloc_4k()
{
	return kern_malloc_4k();
}

//added by mingxuan 2021-8-11
PUBLIC u32 sys_malloc_4k()
{
	return do_malloc_4k();
}

// syscall free
//
PUBLIC int kern_free_4k(void *AddrLin) //modified by mingxuan 2021-8-19
{

	// int phy_addr;

	// phy_addr = get_page_phy_addr(p_proc_current->task.pid, (int)AddrLin); //获取物理页的物理地址

	// clear_pte(p_proc_current->task.pid, (u32)AddrLin);

	//    update_heap_ptr(AddrLin,-1); //update heap pointer
	update_heap_limit(p_proc_current->task.pid, -1); //update heap limit edited by wang 2021.8.26

	// return phy_free_4k(phy_addr);
	return do_munmap(AddrLin, PAGE_SIZE);
}

PUBLIC int do_free_4k(void *AddrLin)
{
	return kern_free_4k(AddrLin);
}

PUBLIC int sys_free_4k()
{
	return do_free_4k((void*)get_arg(1));
}

// mmu for user vmem addr

// find first vma->end > addr
// require: pcb.memmap.vma_lock
PUBLIC struct vmem_area * find_vma(LIN_MEMMAP* mmap, u32 addr) 
{
    struct vmem_area *vma = NULL;
    list_for_each(&mmap->vma_map, vma, vma_list)
    {
        if(vma->end > addr){
            return vma;
        }
    }
    return NULL;
}

PUBLIC page* alloc_user_page(u32 pgoff) {
	page *_page = alloc_pages(ubud, 0);
	atomic_set(&_page->count, 1);
	_page->pg_off = pgoff;
	_page->pg_cache = NULL;
	list_init(&_page->pg_list);
	list_init(&_page->pg_lru);
	memset(_page->pg_buffer, 0, sizeof(_page->pg_buffer));
	return _page;
}



// req. ring 0
PRIVATE void copy_page(page *dst, page *src) {
	u32 dst_vaddr, src_vaddr;
	dst_vaddr = kmap(dst);
	src_vaddr = kmap(src);
	memcpy(dst_vaddr, src_vaddr, PAGE_SIZE);
	kunmap(dst);
	kunmap(src);
	// disp_str("copy:");
	// disp_int(page_to_pfn(src));
	// disp_str("->");
	// disp_int(page_to_pfn(dst));
	// disp_str("\n");
}

// req. ring 0
PUBLIC void zero_page(page *_page) {
	u32 dst_vaddr;
	dst_vaddr = kmap(_page);
	memset(dst_vaddr, 0, PAGE_SIZE);
	kunmap(_page);
}

PUBLIC void copy_from_page(page *_page, void* buf, u32 len, u32 offset) {
	u32 vaddr = kmap(_page);
	memcpy(buf, vaddr + offset, len);
	kunmap(_page);
}

PUBLIC void copy_to_page(page *_page, const void* buf, u32 len, u32 offset) {
	u32 vaddr = kmap(_page);
	memcpy(vaddr + offset, buf, len);
	kunmap(_page);
}

PRIVATE cache_pages* get_page_cache(LIN_MEMMAP* mmap, struct vmem_area* vma) {
	cache_pages *page_cache = NULL;
    if(vma->file) { // file mapped memory
		page_cache = &vma->file->fd_mapping->pages;
    }else if(vma->flags & MAP_PRIVATE){ // process private allocated memory
		page_cache = &mmap->anon_pages;
	}else {
		page_cache = &shm_pages;
	}
	return page_cache;
}

PRIVATE page* find_or_create_page(LIN_MEMMAP* mmap, struct vmem_area* vma, u32 pgoff) {
	cache_pages* page_cache = get_page_cache(mmap, vma);
	lock_or_yield(&page_cache->lock);
    page *_page = find_cache_page(page_cache, pgoff);
	if(_page == NULL) {
		_page = alloc_user_page(pgoff);
		if(vma->file) {
			struct address_space * f_mapping = vma->file->fd_mapping;
			lock_or_yield(&f_mapping->host->lock);
			generic_file_readpage(f_mapping, _page);
			release(&f_mapping->host->lock);
		}else {
			zero_page(_page);
		}
		add_cache_page(page_cache, _page);
	} else {
		get_page(_page);
	}
	release(&page_cache->lock);
	return _page;
}

PRIVATE void free_vma_pages(PROCESS* p_proc, LIN_MEMMAP* mmap, struct vmem_area *vma) {
    u32 nr_pages = (vma->end - vma->start) >> PAGE_SHIFT;
	u32 addr = vma->start;
	for(u32 i = 0; i < nr_pages; i++) {
		u32 phy = get_page_phy_addr(proc2pid(p_proc), addr);
		if(phy) {
			page * _page = pfn_to_page(phy_to_pfn(phy));
			put_page(_page, ((vma->file != NULL)||(vma->flags & MAP_SHARED)));
			clear_pte(proc2pid(p_proc), addr);
		}
		addr += PAGE_SIZE;
	}
}

// 直接为vma申请内存并设置页表, 没有写时复制
// require: pcb.memmap.vma_lock
PUBLIC void prepare_vma(PROCESS* p_proc, LIN_MEMMAP* mmap, struct vmem_area *vma) {
	page * _page = NULL;
	u32 pgoff = vma->pgoff;
    // 页面权限
    u32 pte_attr = PG_P | PG_USU | ((vma->flags & PROT_WRITE)? PG_RWW : PG_RWR);
    for(u32 addr = vma->start; addr < vma->end; addr += PAGE_SIZE, pgoff++) {
		page * content_page = find_or_create_page(mmap, vma, pgoff);
		// 已经增加物理页的引用计数
        // 对于可写的私有文件映射，不能修改文件，所以做一份复制
		if((vma->file) && (vma->flags & PROT_WRITE) && (vma->flags & MAP_PRIVATE)) {
			_page = alloc_user_page(addr>>PAGE_SHIFT);
			copy_page(_page, content_page);
			put_page(content_page, 1);
			add_cache_page(&mmap->anon_pages, _page);
        } else { 
            // 其他情况直接使用page cache的物理页就行
            _page = content_page;
        }
        lin_mapping_phy(addr, pfn_to_phy(page_to_pfn(_page)), 
                proc2pid(p_proc),
                PG_P | PG_USU | PG_RWW,
                pte_attr);
	}
}

PRIVATE void free_vma(PROCESS* p_proc, LIN_MEMMAP* mmap, struct vmem_area *vma) {
    list_remove(&vma->vma_list);
    // clear arch pgtable and free
    free_vma_pages(p_proc, mmap, vma);
    if(vma->file){
        fput(vma->file);
    }
    kern_kfree((u32) vma);
}

// free vma [start, last)
PUBLIC void free_vmas(PROCESS* p_proc, LIN_MEMMAP* mmap, struct vmem_area *start, struct vmem_area *last) {
    struct vmem_area *vma = start, *next;
    while(vma && vma != last) {
        next = list_next(&mmap->vma_map, vma, vma_list);
        free_vma(p_proc, mmap, vma);
        vma = next;
    }
}

PUBLIC void memmap_copy(PROCESS* p_parent, PROCESS* p_child) {
	LIN_MEMMAP* old_mmap = proc_memmap(p_parent);
	LIN_MEMMAP* new_mmap = proc_memmap(p_child);
	*new_mmap = *old_mmap;
	init_cache_page(&new_mmap->anon_pages); 
	// if COW enabled:
	// as for fork, child mmap clear and all page tbl set read only, and inc page reference count
	// after fork, child share the same phy page with the parent,
	// when write then page fault occurs, COW will copy the origin page to a new anon page
	list_init(&new_mmap->vma_map);
	struct vmem_area *vma, *vm;
	lock_or_yield(&old_mmap->vma_lock);
	list_for_each(&old_mmap->vma_map, vma, vma_list) {
		vm = (struct vmem_area *)kern_kmalloc(sizeof(struct vmem_area));
		*vm = *vma;
		list_init(&vm->vma_list);
		if(vm->file) {
			fget(vm->file);
		}
		list_add_last(&vm->vma_list, &new_mmap->vma_map);
		u32 pte_attr = PG_P | PG_USU | ((vma->flags & PROT_WRITE)? PG_RWW : PG_RWR);
		for(u32 addr = vma->start; addr < vma->end; addr += PAGE_SIZE) {
			u32 phy = get_page_phy_addr(proc2pid(p_parent), addr);
			if(phy) {
				page *pte_page = pfn_to_page(phy_to_pfn(phy));
				#ifdef MMU_COW
				// 写时复制，将父子进程页表同时置为只读
					get_page(pte_page); // add reference
					lin_mapping_phy(addr, phy, 
						proc2pid(p_parent), 
						PG_P | PG_USU | PG_RWW,
						PG_P | PG_USU | PG_RWR); // set read only
					lin_mapping_phy(addr, phy, 
						proc2pid(p_child), 
						PG_P | PG_USU | PG_RWW,
						PG_P | PG_USU | PG_RWR); 
				#else 
				// 无写时复制， 根据权限决定，只读的页面共享，可写的私有页面申请新页面并复制
					page *_page = NULL;
					if((vma->flags & MAP_PRIVATE) && (vma->flags & PROT_WRITE)) {
						_page = alloc_user_page(addr>>PAGE_SHIFT);
						copy_page(_page, pte_page);
						add_cache_page(&new_mmap->anon_pages, _page);
					} else { 
						// 其他情况直接使用父进程的物理页就行,要增加父进程物理页的引用计数
						get_page(pte_page);
						_page = pte_page;
					}
					lin_mapping_phy(addr, pfn_to_phy(page_to_pfn(_page)), 
							proc2pid(p_child),
							PG_P | PG_USU | PG_RWW,
							pte_attr);
				#endif
			}
		}
	}
	release(&old_mmap->vma_lock);
}

void memmap_clear(PROCESS* p_proc) {
	LIN_MEMMAP* mmap = proc_memmap(p_proc);
	free_vmas(p_proc, mmap, list_front(&mmap->vma_map, struct vmem_area, vma_list), NULL);
}

// handle generic mm fault, return 0 if handle normal ok, -1 for error
int handle_mm_fault(LIN_MEMMAP* mmap, u32 vaddr, int flag) {
	#ifndef MMU_COW
		disp_color_str("error: no cow, this handler is unreachable!", 0x74);
	#endif
	struct vmem_area *vma = find_vma(mmap, vaddr);
	if(vma && vaddr >= vma->start) {
		u32 pgoff = vma->pgoff + ((vaddr - vma->start) >> PAGE_SHIFT);
		page * _page = NULL;
		u32 attr = PG_P|PG_USU;// 这里目前没有做架构分离
		u32 phy = get_page_phy_addr(proc2pid(p_proc_current), vaddr);
		page *pte_page = NULL;
		if(phy != NULL){
			pte_page = pfn_to_page(phy_to_pfn(phy));
		}
		if(flag & FAULT_NOPAGE) { // handler should check page and set pte
			if(pte_page == NULL) {
				// 页表完全没有设置物理页，需要找到正确的物理页
				_page = find_or_create_page(mmap, vma, pgoff);
				if(vma->file && vma->flags & MAP_PRIVATE) {
					attr |= PG_RWR;
				} else {
					attr |= ((vma->flags & PROT_WRITE)? PG_RWW: PG_RWR);
				}
			} else { // pte 存在物理地址，但是页面不存在，所以页面被换出了
				// 由于暂时不存在相应机制，暂不处理
				// 如果运行到此处, 说明页表存在问题
				disp_str("error: page swapped? or just wrong pte?");

			}
		}else if((flag & FAULT_WRITE) && (vma->flags & PROT_WRITE)) {
			if(pte_page == NULL){ disp_str("error: no page for page present fault");} // error
			if(vma->flags & MAP_PRIVATE) {// file private RW: COW now
				_page = alloc_user_page(vaddr>>PAGE_SHIFT);
				copy_page(_page, pte_page);
				add_cache_page(&mmap->anon_pages, _page);
				attr |= PG_RWW;
				put_page(pte_page, vma->file != NULL);
			}
		}
		
		if(_page) {// now new page is ready
			// disp_int(proc2pid(p_proc_current));
			// disp_str(":");
			// disp_int(vaddr&0xFFFFF000);
			// disp_str("->");
			// disp_int(page_to_pfn(_page));
			// disp_str("\n");
			lin_mapping_phy(vaddr, pfn_to_phy(page_to_pfn(_page)), 
				proc2pid(p_proc_current), PG_P | PG_USU | PG_RWW, attr);
			return 0;
		}
	}
	return -1; // invalid addr;
}