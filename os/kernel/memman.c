#include "memman.h"
#include "buddy.h" //added by mingxuan 2021-3-8
#include "pagetable.h"
#include "shm.h"
#include "fs.h"
#include "proto.h"
#include "kmalloc.h"
#include "string.h"

int big_kernel = 0;        //当big_kernel=1时，表示大内核，big_kernel=0表示小内核，added by wang 2021.8.16
u32 kernel_size = 0;       //表示内核大小的全局变量，added by wang 2021.8.27
u32 kernel_code_size = 0;  //为内核代码数据分配的内存大小，     added by wang 2021.8.27
u32 test_phy_mem_size = 0; //检测到的物理机的物理内存的大小，    added by wang 2021.8.27
u32 MemInfo[256] = {0}; //存放FMIBuff后1k内容
page mem_map[ALL_PAGES];

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


PUBLIC void phy_mapping_4k(u32 phy_addr) {
	// disp_str(" m");
	// disp_int(phy_addr);
	page *page = pfn_to_page(phy_to_pfn(phy_addr));
	atomic_inc(&page->count);
}
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
PUBLIC int kern_mapping_4k(u32 AddrLin, u32 pid, u32 phyAddr, u32 pte_attribute) {
	if (phyAddr != MAX_UNSIGNED_INT) {
		phy_mapping_4k(phyAddr);
	}
	return lin_mapping_phy(AddrLin, phyAddr, pid, PG_P | PG_USU | PG_RWW, pte_attribute);
}

PUBLIC int ker_umalloc_4k(u32 AddrLin, u32 pid, u32 pte_attribute)
{

	//不用判AddrLin是否4K对齐，lin_mapping_phy会为非4k对齐的线性地址分配物理页

	if (AddrLin > KernelLinBase)
	{
		disp_color_str("ker_umalloc_4k: address error ", 0x74);
		return -1;
	}

	return kern_mapping_4k(AddrLin, pid, MAX_UNSIGNED_INT, pte_attribute);
}

//added by wang 2021.8.24

PUBLIC int ker_ufree_4k(u32 pid, u32 AddrLin)
{
	u32 phy_addr = 0;

	if (AddrLin > KernelLinBase)
	{

		disp_color_str("ker_ufree_4k: address error ", 0x74);
		return -1;
	}
	else //释放物理页
	{
		phy_addr = get_page_phy_addr(pid, AddrLin);
		phy_free_4k(phy_addr);
		clear_pte(pid, AddrLin);
		return 0;
	}
}

PUBLIC u32 kern_malloc_4k() //modified by mingxuan 2021-8-19
{

	u32 AddrLin;

	AddrLin = get_heap_limit(p_proc_current->task.pid);
	update_heap_limit(p_proc_current->task.pid, 1);
	kern_mapping_4k(AddrLin, p_proc_current->task.pid, 
		MAX_UNSIGNED_INT, PG_P | PG_USU | PG_RWW);
	return AddrLin;
}

//added by mingxuan 2021-8-19
PUBLIC u32 do_malloc_4k()
{
	kern_malloc_4k();
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

	int phy_addr;

	phy_addr = get_page_phy_addr(p_proc_current->task.pid, (int)AddrLin); //获取物理页的物理地址

	clear_pte(p_proc_current->task.pid, (u32)AddrLin);

	//    update_heap_ptr(AddrLin,-1); //update heap pointer
	update_heap_limit(p_proc_current->task.pid, -1); //update heap limit edited by wang 2021.8.26

	return phy_free_4k(phy_addr);
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


PRIVATE inline void get_user_page(page *_page) {
	atomic_inc(&_page->count);
}

PRIVATE int put_user_page(page *_page) {
	if (atomic_dec_and_test(&_page->count)) {
		list_remove(&_page->pg_list);
		return free_pages(ubud, _page, 0);
	}
	return 0;
}

// find first vma->end > addr
struct vmem_area * find_vma(LIN_MEMMAP* mmap, u32 addr) 
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

PRIVATE page* alloc_user_page(struct list_node *page_list_head, u32 pgoff) {
	page *_page = alloc_pages(ubud, 0);
	atomic_set(&_page->count, 1);
    _page->pg_off = pgoff;
	list_init(&_page->pg_list);
	page* next = NULL;
	list_for_each(page_list_head, next, pg_list)
	{
		if(next->pg_off > pgoff)break;
	}
	list_add_before(&_page->pg_list, (next)?(&next->pg_list):page_list_head);
}

// req. ring 0
PRIVATE void copy_page(page *dst, page *src) {
	u32 dst_vaddr, src_vaddr;
	dst_vaddr = kmap(dst);
	src_vaddr = kmap(src);
	memcpy(dst_vaddr, src_vaddr, PAGE_SIZE);
	kunmap(dst);
	kunmap(src);
}

// req. ring 0
PRIVATE void zero_page(page *_page) {
	u32 dst_vaddr;
	dst_vaddr = kmap(_page);
	memset(dst_vaddr, 0, PAGE_SIZE);
	kunmap(_page);
}

void copy_from_page(page *_page, void* buf, u32 len, u32 offset) {
	u32 vaddr = kmap(_page);
	memcpy(buf, vaddr + offset, len);
	kunmap(_page);
}

void copy_to_page(page *_page, const void* buf, u32 len, u32 offset) {
	u32 vaddr = kmap(_page);
	memcpy(vaddr + offset, buf, len);
	kunmap(_page);
}

// find cached page
PRIVATE page* find_page(LIN_MEMMAP* mmap, struct file_desc *file, u32 pgoff) {
	page *_page;
    if(file) {
		_page = list_find_key(file->fd_mapping, page, pg_list, pg_off, pgoff);
    }else {
		_page = list_find_key(&mmap->anon_pages, page, pg_list, pg_off, pgoff);
	}
	if(_page->pg_off != pgoff){
		disp_str("error: mismatch page");
	}
	return _page;
}

PRIVATE page* find_or_get_page(LIN_MEMMAP* mmap, struct file_desc *file, u32 pgoff) {
    page *_page = find_page(mmap, file, pgoff);
	if(_page == NULL) {
		if(file) {
			_page = alloc_user_page(file->fd_mapping, pgoff);
			// read page

		}else {
			_page = alloc_user_page(&mmap->anon_pages, pgoff);
			zero_user_page(_page);
		}
	}
	return _page;
}

PRIVATE void free_vma_pages(LIN_MEMMAP* mmap, struct vmem_area *vma) {
    u32 nr_pages = (vma->end - vma->start) >> PAGE_SHIFT;
	u32 addr = vma->start;
	for(u32 i = 0; i < nr_pages; i++) {
		page * _page = pfn_to_page(phy_to_pfn(get_page_phy_addr(proc2pid(p_proc_current), addr)));
		put_user_page(_page);
		clear_pte(proc2pid(p_proc_current), addr);
		addr += PAGE_SIZE;
	}
}

void free_vma(LIN_MEMMAP* mmap, struct vmem_area *vma) {
    list_remove(&vma->vma_list);
    // clear arch pgtable and free
    free_vma_pages(mmap, vma);
    if(vma->file){
        fput(vma->file);
    }
    kern_kfree((u32) vma);
}

// free vma [start, last)
void free_vmas(LIN_MEMMAP* mmap, struct vmem_area *start, struct vmem_area *last) {
    struct vmem_area *vma, *next;
    while(vma && vma != last) {
        next = list_next(&mmap->vma_map, vma, vma_list);
        free_vma(mmap, vma);
        vma = next;
    }
}

void memmap_copy(PROCESS* p_parent, PROCESS* p_child) {
	LIN_MEMMAP* old_mmap = &p_parent->task.memmap;
	LIN_MEMMAP* new_mmap = &p_child->task.memmap;
	*new_mmap = *old_mmap;
	list_init(&new_mmap->anon_pages); 
	// as for fork, child mmap clear and all page tbl set read only, and inc page reference count
	// after fork, child share the same phy page with the parent,
	// when write then page fault occurs, COW will copy the origin page to a new anon page
	list_init(&new_mmap->vma_map);
	struct vmem_area *vma, *vm;
	list_for_each(&old_mmap->vma_map, vma, vma_list) {
		vm = (struct vmem_area *)kern_kmalloc(sizeof(struct vmem_area));
		*vm = *vma;
		list_init(&vm->vma_list);
		list_add_last(&vm->vma_list, &new_mmap->vma_map);
		for(u32 addr = vma->start; addr < vma->end; addr += PAGE_SIZE) {
			page *_page = pfn_to_page(phy_to_pfn(
				get_page_phy_addr(proc2pid(p_parent), addr)));
			get_user_page(_page); // add reference
			lin_mapping_phy(addr, MAX_UNSIGNED_INT, 
				proc2pid(p_parent), 
				PG_P | PG_USU | PG_RWW,
				PG_P | PG_USU | PG_RWR); // set read only
		}
	}
	// copy pde(cr3)? no, child page fault will set new page for child,
	// if read, the two proc share same phy page
	// if write, COW will do the copy 
}

void memmap_clear(PROCESS* p_proc) {
	LIN_MEMMAP* mmap = &p_proc->task.memmap;
	struct vmem_area *vma;
	free_vmas(mmap, list_front(&mmap->vma_map, struct vmem_area, vma_list), NULL);
	// copy pde(cr3)? no, child page fault will set new page for child,
	// if read, the two proc share same phy page
	// if write, COW will do the copy 
}

// handle generic mm fault, return 0 if handle normal ok, -1 for error
void handle_mm_fault(LIN_MEMMAP* mmap, u32 vaddr, int flag) {
	struct vmem_area *vma = find_vma(mmap, vaddr);
	if(vma && vaddr >= vma->start) {
		u32 pgoff = vma->pgoff + ((vaddr - vma->start) >> PAGE_SHIFT);
		page * _page = NULL;
		u32 attr = PG_P|PG_USU;// 这里目前没有做架构分离
		if(flag & FAULT_NOPAGE) { // handler should check page and set pte
			_page = find_or_get_page(mmap, vma->file, pgoff);
			if(vma->file && vma->flags & MAP_PRIVATE) {
				attr |= PG_RWR;
			}else {
				attr |= ((vma->flags & PROT_WRITE)? PG_RWW: PG_RWR);
			}
		}else if((flag & FAULT_WRITE) && (vma->flags & PROT_WRITE)) {
			page *old_page = pfn_to_page(phy_to_pfn(get_page_phy_addr(proc2pid(p_proc_current), vaddr)));
			// if(old_page == NULL){ disp_str("error: no page for page present fault");} // error
			if(vma->flags & MAP_PRIVATE) {// file private RW: COW now
				_page = alloc_user_page(&mmap->anon_pages, );
				copy_user_page(_page, old_page);
				attr |= PG_RWW;
				put_user_page(old_page);
			}
		}
		
		if(_page) {// now new page is ready
			lin_mapping_phy(vaddr, pfn_to_phy(page_to_pfn(_page)), 
				proc2pid(p_proc_current), PG_P | PG_USU | PG_RWW, attr);
		}
	}
	return -1; // invalid addr;
}