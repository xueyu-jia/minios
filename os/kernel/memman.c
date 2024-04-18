#include "memman.h"
#include "buddy.h" //added by mingxuan 2021-3-8
#include "pagetable.h"
#include "shm.h"
#include "proto.h"
#include "string.h"

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
	if(AddrLin == 0x4801100){
		while(1);
	}
	update_heap_limit(p_proc_current->task.pid, 1);

	//AddrLin = p_proc_current->task.memmap.heap_lin_limit;
	//AddrLin = *(u32 *)p_proc_current->task.memmap.heap_lin_limit;	//modified by mingxuan 2021-8-19
	//disp_int(p_proc_current->task.memmap.heap_lin_base);
	//disp_str("limit=");
	//disp_int(p_proc_current->task.memmap.heap_lin_limit);
	//p_proc_current->task.memmap.heap_lin_limit += num_4K;

	// lin_mapping_phy(AddrLin,				  //线性地址					//add by visual 2016.5.9
	// 				MAX_UNSIGNED_INT,		  //物理地址
	// 				p_proc_current->task.pid, //进程pid					//edit by visual 2016.5.19
	// 				PG_P | PG_USU | PG_RWW,	  //页目录的属性位
	// 				PG_P | PG_USU | PG_RWW);  //页表的属性位
	kern_mapping_4k(AddrLin, p_proc_current->task.pid, MAX_UNSIGNED_INT, PG_P | PG_USU | PG_RWW);
	//update_heap_ptr(vaddr, 1);
	//        Scan_free_area(ubud);
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
PUBLIC int sys_free_4k()
{
	return do_free_4k((void*)get_arg(1));
}

PUBLIC int do_free_4k(void *AddrLin)
{
	return kern_free_4k(AddrLin);
}

//PUBLIC int do_free_4k(void* AddrLin)
PUBLIC int kern_free_4k(void *AddrLin) //modified by mingxuan 2021-8-19
{

	int phy_addr;

	phy_addr = get_page_phy_addr(p_proc_current->task.pid, (int)AddrLin); //获取物理页的物理地址

	clear_pte(p_proc_current->task.pid, (u32)AddrLin);

	//    update_heap_ptr(AddrLin,-1); //update heap pointer
	update_heap_limit(p_proc_current->task.pid, -1); //update heap limit edited by wang 2021.8.26

	return phy_free_4k(phy_addr);
}