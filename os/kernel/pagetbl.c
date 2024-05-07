/*************************************************************
*页式管理相关代码 add by visual 2016.4.19
**************************************************************/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "proto.h"
#include "buddy.h"
#include "memman.h"
#include "pagetable.h"
#include "spinlock.h"

//to determine if a page fault is reparable. added by xw, 18/6/11
u32 cr2_save;
u32 cr2_count = 0;
u32 kernel_pde_phy;
u32 nr_kmapping_pages = 0;
PRIVATE u32 kmapping_pages[KernelLinMapMaxPage];
PRIVATE struct spinlock kmap_lock;
/*======================================================================*
                           switch_pde			added by xw, 17/12/11
 *switch the page directory table after schedule() is called
 *======================================================================*/
PUBLIC void switch_pde()
{
	cr3_ready = p_proc_current->task.cr3;
}

/***************************地址转换过程***************************
*
*第一步，CR3包含着页目录的起始地址，用32位线性地址的最高10位A31~A22作为页目录的页目录项的索引，
*将它乘以4，与CR3中的页目录的起始地址相加，形成相应页表的地址。
*
*第二步，从指定的地址中取出32位页目录项，它的低12位为0，这32位是页表的起始地址。
*用32位线性地址中的A21~A12位作为页表中的页面的索引，将它乘以4，与页表的起始地址相加，形成32位页面地址。
*
*第三步，将A11~A0作为相对于页面地址的偏移量，与32位页面地址相加，形成32位物理地址。
*************************************************************************/

/*======================================================================*
                          get_pde_index		add by visual 2016.4.28
 *======================================================================*/
PRIVATE inline u32 get_pde_index(u32 AddrLin)
{							//由 线性地址 得到 页目录项编号
	return (AddrLin >> 22); //高10位A31~A22
}

/*======================================================================*
                          get_pte_index		add by visual 2016.4.28
 *======================================================================*/
PRIVATE inline u32 get_pte_index(u32 AddrLin)
{										   //由 线性地址 得到 页表项编号
	return (((AddrLin)&0x003FFFFF) >> 12); //中间10位A21~A12,0x3FFFFF = 0000 0000 0011 1111 1111 1111 1111 1111
}

/*======================================================================*
                          get_pde_phy_addr	add by visual 2016.4.28
 *======================================================================*/
PRIVATE u32 get_pde_phy_addr(u32 pid)
{ //获取页目录物理地址
	if (proc_table[pid].task.cr3 == 0)
	{ //还没有初始化页目录
		return NULL;
	}
	else
	{
		return ((proc_table[pid].task.cr3) & 0xFFFFF000);
	}
}

/*======================================================================*
                          get_pte_phy_addr	add by visual 2016.4.28
 *======================================================================*/
PRIVATE inline u32 get_pte_phy_addr(u32 PageDirPhyAddr,		//页目录物理地址		//edit by visual 2016.5.19
								   u32 AddrLin) 			//线性地址
{															//获取该线性地址所属页表的物理地址
	if (0 == PageDirPhyAddr)								//异常处理, added by mingxuan 2021-1-29
		disp_str("Get PageDir Physical Address Error!\n");

	return (*((u32 *)K_PHY2LIN(PageDirPhyAddr) + get_pde_index(AddrLin))) & 0xFFFFF000; //先找到该进程页目录首地址，然后计算出该线性地址对应的页目录项，再访问,最后注意4k对齐
}

/*======================================================================*
                          get_page_phy_addr	add by visual 2016.5.9  refact 20240405 jiangfeng
 *======================================================================*/
PRIVATE inline u32 get_page_phy_addr_nopid(u32 PageTblPhyAddr,			 //页表物理地址				//edit by visual 2016.5.19
									u32 AddrLin)		 //线性地址
{														 //获取该线性地址对应的物理页物理地址

	if (0 == PageTblPhyAddr)							 //异常处理, added by mingxuan 2021-1-29
		disp_str("Get PageTbl Physical Address Error!\n");

	return (*((u32 *)K_PHY2LIN(PageTblPhyAddr) + get_pte_index(AddrLin))) & 0xFFFFF000;
}

/*======================================================================*
                          pte_exist		add by visual 2016.4.28
 *======================================================================*/
PUBLIC u32 pte_exist(u32 PageDirPhyAddr,													//页目录物理地址
					 u32 AddrLin)															//线性地址
{																							//判断 有没有 页表
	if ((0x00000001 & (*((u32 *)K_PHY2LIN(PageDirPhyAddr) + get_pde_index(AddrLin)))) == 0) //先找到该进程页目录,然后计算出该线性地址对应的页目录项,访问并判断其是否存在
	{																						//标志位为0，不存在
		return 0;
	}
	else
	{
		return 1;
	}
}

/*======================================================================*
                          phy_exist		add by visual 2016.4.28
 *======================================================================*/
PUBLIC u32 phy_exist(u32 PageTblPhyAddr, //页表物理地址
					 u32 AddrLin)		 //线性地址
{										 //判断 该线性地址 有没有 对应的 物理页
	if ((0x00000001 & (*((u32 *)K_PHY2LIN(PageTblPhyAddr) + get_pte_index(AddrLin)))) == 0)
	{ //标志位为0，不存在
		return 0;
	}
	else
	{
		return 1;
	}
}

PUBLIC u32 get_page_phy_addr(u32 pid, u32 AddrLin) {
	u32 pde_phy = get_pde_phy_addr(pid);
	if(!pde_phy || (pte_exist(pde_phy, AddrLin) == 0)) return NULL;
	u32 pte_phy = get_pte_phy_addr(pde_phy, AddrLin);
	if(!pte_phy) return NULL;
	return get_page_phy_addr_nopid(pte_phy, AddrLin);
}

/*======================================================================*
                          write_page_pde		add by visual 2016.4.28
 *======================================================================*/
PUBLIC void write_page_pde(u32 PageDirPhyAddr, //页目录物理地址
						   u32 AddrLin,		   //线性地址
						   u32 TblPhyAddr,	   //要填写的页表的物理地址（函数会进行4k对齐）
						   u32 Attribute)	   //属性
{											   //填写页目录
	(*((u32 *)K_PHY2LIN(PageDirPhyAddr) + get_pde_index(AddrLin))) = (TblPhyAddr & 0xFFFFF000) | Attribute;
	//进程页目录起始地址+每一项的大小*所属的项
}

/*======================================================================*
                          write_page_pte		add by visual 2016.4.28
 *======================================================================*/
PUBLIC void write_page_pte(u32 TblPhyAddr, //页表物理地址
						   u32 AddrLin,	   //线性地址
						   u32 PhyAddr,	   //要填写的物理页物理地址(任意的物理地址，函数会进行4k对齐)
						   u32 Attribute)  //属性
{										   //填写页目录，会添加属性
	(*((u32 *)K_PHY2LIN(TblPhyAddr) + get_pte_index(AddrLin))) = (PhyAddr & 0xFFFFF000) | Attribute;
	//页表起始地址+一项的大小*所属的项
}

/*======================================================================*
                           init_proc_page		add by visual 2016.4.19  modifyed: jiangfeng 2024.4
*该函数只初始化了进程的高端（内核端）地址页表
 *======================================================================*/
PUBLIC int init_proc_page(u32 pid)
{ //页表初始化函数

	u32 AddrLin, pde_addr_phy_temp, pte_addr_phy_temp, err_temp;

	//pde_addr_phy_temp = do_kmalloc_4k();//为页目录申请一页
	pde_addr_phy_temp = phy_kmalloc_4k(); //为页目录申请一页	//modified by mingxuan 2021-8-16

	memset((void *)K_PHY2LIN(pde_addr_phy_temp), 0, num_4K); //add by visual 2016.5.26

	if (pde_addr_phy_temp < 0 || (pde_addr_phy_temp & 0x3FF) != 0) //add by visual 2016.5.9
	{
		disp_color_str("init_proc_page Error:pde_addr_phy_temp", 0x74);
		return -1;
	}

	proc_table[pid].task.cr3 = pde_addr_phy_temp; //初始化了进程表中cr3寄存器变量，属性位暂时不管

	/*********************页表初始化部分*********************************/
	u32 kernel_pde_offset = KernelLinBase/num_4M * 4;
	memcpy((void*)(K_PHY2LIN(pde_addr_phy_temp) + kernel_pde_offset), 
		(void*)(K_PHY2LIN(kernel_pde_phy) + kernel_pde_offset), num_4K - kernel_pde_offset);

	return 0;
}

//added by mingxuan 2021-8-25
PUBLIC int init_kernel_page()
{
	//第一步: 生成一张内核用的页目录表
	u32 kernel_pde_addr_phy = (u32)phy_kmalloc_4k();
	kernel_pde_phy = kernel_pde_addr_phy;
	memset((void *)K_PHY2LIN(kernel_pde_addr_phy), 0, num_4K); //by qianglong
	//第二步: 初始化3G~3G+kernel_size的内核映射
	u32 AddrLin = 0, phy_addr = 0;

	//delete by sundong 2023.3.8 kernel中低端页表没有发挥作用，因此此处删掉对低端页表的映射
	/*
	//建立对低端0~kernel_size内核的映射
	for (AddrLin = 0, phy_addr = 0; AddrLin < 0 + kernel_size; AddrLin += num_4K, phy_addr += num_4K)
	{												   //只初始化内核部分，3G后的线性地址映射到物理地址开始处
		int err_temp = lin_mapping_phy_nopid(AddrLin,  //线性地址					//add by visual 2016.5.9
											 phy_addr, //物理地址
											 kernel_pde_addr_phy,
											 PG_P | PG_USS | PG_RWW,  //页目录的属性位（系统权限）			//edit by visual 2016.5.26
											 PG_P | PG_USS | PG_RWW); //页表的属性位（系统权限）				//edit by visual 2016.5.17
		if (err_temp != 0)
		{
			disp_color_str("init kernel page Error:lin_mapping_phy", 0x74);
			return -1;
		}
	}
	*/
	// jiangfeng 20240328 给内核pde中需要的位置分配
	for (AddrLin = KernelLinBase; AddrLin < KernelLinMapLimit; AddrLin += num_4M) {
		phy_addr = (u32)phy_kmalloc_4k(); //为页表申请一页	//modified by mingxuan 2021-8-16

		memset((void *)K_PHY2LIN(phy_addr), 0, num_4K); //add by visual 2016.5.26

		if (phy_addr < 0 || (phy_addr & 0x3FF) != 0) //add by visual 2016.5.9
		{
			disp_color_str("lin_mapping_phy Error:pte_addr_phy", 0x74);
			return -1;
		}

		write_page_pde(kernel_pde_addr_phy,   //页目录物理地址
					   AddrLin,		   //线性地址
					   phy_addr,   //页表物理地址
					   PG_P | PG_USS | PG_RWW); //属性（系统权限）
	}

	//建立3G~3G+kernel_size的内核映射
	for (AddrLin = KernelLinBase, phy_addr = 0; AddrLin < KernelLinBase + kernel_size; AddrLin += num_4K, phy_addr += num_4K)
	{												   //只初始化内核部分，3G后的线性地址映射到物理地址开始处
		int err_temp = lin_mapping_phy_nopid(AddrLin,  //线性地址					//add by visual 2016.5.9
											 phy_addr, //物理地址
											 kernel_pde_addr_phy,
											 PG_P | PG_USS | PG_RWW,  //页目录的属性位（系统权限）			//edit by visual 2016.5.26
											 PG_P | PG_USS | PG_RWW); //页表的属性位（系统权限）				//edit by visual 2016.5.17
		if (err_temp != 0)
		{
			disp_color_str("init kernel page Error:lin_mapping_phy", 0x74);
			return -1;
		}
	}

	//第三步：更换cr3
	__asm__(
		"mov %0, %%eax\n"
		"mov %%eax, %%cr3\n"
		:
		: "m"(kernel_pde_addr_phy));
}

//modified by xw, 18/6/11; mingxuan 2021-1-11
PUBLIC void page_fault_handler(u32 vec_no,	 //异常编号，此时应该是14，代表缺页异常
							   u32 err_code, //错误码
							   u32 eip,		 //导致缺页的指令的线性地址
							   u32 cs,		 //发生错误时的代码段寄存器内容
							   u32 eflags)	 //时发生错误的标志寄存器内容
{											 //缺页中断处理函数
	u32 pde_addr_phy_temp;
	u32 pte_addr_phy_temp;
	u32 phy_addr;
	u32 cr2;

	cr2 = read_cr2();

	//if page fault happens in kernel, it's an error.
	// if (kernel_initial == 1) // 事实上，内核线性地址造成的缺页中断都不正常吧
	if (cr2 >= KernelLinBase)
	{
		goto fatal;
	}
	// if (cr2 == cr2_save) 同一个地址允许无条件缺页5次，难免太抽象了吧
	// {
	// 	cr2_count++;
	// 	if (cr2_count == 5)
	// 	{
	// 		goto fatal;
	// 	}
	// }
	// else
	// {
	// 	cr2_save = cr2;
	// 	cr2_count = 0;
	// }
	//获取该进程页目录物理地址
	pde_addr_phy_temp = get_pde_phy_addr(p_proc_current->task.pid);
	//获取该线性地址对应的页表的物理地址
	pte_addr_phy_temp = get_pte_phy_addr(pde_addr_phy_temp, cr2);
	// 这种粗暴的缺页处理办法会导致内存混乱的，连页表里保存的物理地址是什么都不检查就置为有效了
	// if (0 == pte_exist(pde_addr_phy_temp, cr2))
	// { //页表不存在
	// 	// disp_color_str("[Pde Fault!]",0x74);	//灰底红字
	// 	(*((u32 *)K_PHY2LIN(pde_addr_phy_temp) + get_pde_index(cr2))) |= PG_P;
	// 	// disp_color_str("[Solved]",0x74);
	// }
	// else
	// { //只是缺少物理页
	// 	// disp_color_str("[Pte Fault!]",0x74);	//灰底红字
	// 	(*((u32 *)K_PHY2LIN(pte_addr_phy_temp) + get_pte_index(cr2))) |= PG_P;
	// 	// disp_color_str("[Solved]",0x74);
	// }

	// 目前MiniOS还没有交换页面的选项
	// 2024.5 mm 重构， page fault handler 最后交由架构无关的相关处理函数
	int fault_flag = 0;
	if (0 == pte_exist(pde_addr_phy_temp, cr2) || 0 == phy_exist(pte_addr_phy_temp, cr2)) {
		fault_flag |= FAULT_NOPAGE;
	} else { //此处有物理页，那是为什么缺页中断？
		phy_addr = (*((u32 *)K_PHY2LIN(pte_addr_phy_temp) + get_pte_index(cr2)));
		u32 attr = phy_addr&0xFFF;
		phy_addr = phy_addr&0xFFFFF000;
		// 检查是否为合法物理地址?
		if((phy_addr) < ((big_kernel)?KUWALL2:KUWALL1)) { // 错误的物理页，这不对吧, 
			goto fatal;
		} else {
			// 检查权限
			if((err_code & 2) && (!(attr & PG_RWW))) {// 缺页原因为写入，页面无写权限
				fault_flag |= FAULT_WRITE;
			}
		}
	}
	#ifdef MMU_COW
	if(handle_mm_fault(proc_memmap(p_proc_current), cr2, fault_flag) == 0) {
		refresh_page_cache();
		return;
	}
	#endif	
fatal:
	disp_str("\n");
	disp_color_str("Page Fault\n", 0x74);
	disp_color_str("eip=", 0x74); //灰底红字
	disp_int(eip);
	disp_color_str("eflags=", 0x74);
	disp_int(eflags);
	disp_color_str("cs=", 0x74);
	disp_int(cs);
	disp_color_str("err_code=", 0x74);
	disp_int(err_code);
	disp_color_str("Cr2=", 0x74); //灰底红字
	disp_int(cr2);
	proc_backtrace();
	// halt();
	// do_exit(-1);
}


//added by mingxuan 2021-8-25
PUBLIC int lin_mapping_phy_nopid(u32 AddrLin,  //线性地址
								 u32 phy_addr, //物理地址,若为MAX_UNSIGNED_INT(0xFFFFFFFF)，则表示需要由该函数判断是否分配物理地址，否则将phy_addr直接和AddrLin建立映射
								 u32 kernel_pde_addr_phy,
								 u32 pde_Attribute, //页目录中的属性位
								 u32 pte_Attribute) //页表中的属性位
{
	u32 pte_addr_phy;
	u32 pde_addr_phy = kernel_pde_addr_phy; //add by visual 2016.5.19

	if (0 == pte_exist(pde_addr_phy, AddrLin))
	{ //页表不存在，创建一个，并填进页目录中
		//pte_addr_phy = (u32)do_kmalloc_4k(); //为页表申请一页
		pte_addr_phy = (u32)phy_kmalloc_4k(); //为页表申请一页	//modified by mingxuan 2021-8-16

		memset((void *)K_PHY2LIN(pte_addr_phy), 0, num_4K); //add by visual 2016.5.26

		if (pte_addr_phy < 0 || (pte_addr_phy & 0x3FF) != 0) //add by visual 2016.5.9
		{
			disp_color_str("lin_mapping_phy Error:pte_addr_phy", 0x74);
			return -1;
		}

		write_page_pde(pde_addr_phy,   //页目录物理地址
					   AddrLin,		   //线性地址
					   pte_addr_phy,   //页表物理地址
					   pde_Attribute); //属性
	}
	else
	{ //页表存在，获取该页表物理地址
		pte_addr_phy = (*((u32 *)K_PHY2LIN(pde_addr_phy) + get_pde_index(AddrLin))) & 0xFFFFF000;
	}

	if (MAX_UNSIGNED_INT == phy_addr) //add by visual 2016.5.19
	{								  //由函数申请内存
		if (0 == phy_exist(pte_addr_phy, AddrLin))
		{ //无物理页，申请物理页并修改phy_addr
			if (AddrLin >= K_PHY2LIN(0)){
				//phy_addr = do_kmalloc_4k();//从内核物理地址申请一页
				phy_addr = phy_kmalloc_4k(); //从内核物理地址申请一页	//modified by mingxuan 2021-8-16
			}
			else
			{
				//disp_str("%");
				//phy_addr = do_malloc_4k();//从用户物理地址空间申请一页
				phy_addr = phy_malloc_4k(); //从用户物理地址空间申请一页	//modified by mingxuan 2021-8-14
			}
		}
		else
		{
			//有物理页，什么也不做,直接返回，必须返回
			// return 0;
			// 20240405 对于已经存在的页表项可能需要更新权限属性，故不能直接返回
			phy_addr = get_page_phy_addr_nopid(pte_addr_phy, AddrLin);
		}
	}
	else
	{	//指定填写phy_addr
		//不用修改phy_addr
	}

	if (phy_addr < 0 || (phy_addr & 0x3FF) != 0)
	{
		disp_color_str("lin_mapping_phy:phy_addr ERROR", 0x74);
		return -1;
	}
	if(phy_addr == 0) {
		pte_Attribute &= ~PG_P;
	}

	write_page_pte(pte_addr_phy,   //页表物理地址
				   AddrLin,		   //线性地址
				   phy_addr,	   //物理页物理地址
				   pte_Attribute); //属性
	refresh_page_cache();

	return 0;
}

/*======================================================================*
*                          lin_mapping_phy		add by visual 2016.5.9
*将线性地址映射到物理地址上去,函数内部会分配物理地址
*======================================================================*/
PUBLIC int lin_mapping_phy(u32 AddrLin,		  //线性地址
						   u32 phy_addr,	  //物理地址,若为MAX_UNSIGNED_INT(0xFFFFFFFF)，则表示需要由该函数判断是否分配物理地址，否则将phy_addr直接和AddrLin建立映射
						   u32 pid,			  //进程pid						//edit by visual 2016.5.19
						   u32 pde_Attribute, //页目录中的属性位
						   u32 pte_Attribute) //页表中的属性位
{
	u32 pde_addr_phy = get_pde_phy_addr(pid); //add by visual 2016.5.19
	return lin_mapping_phy_nopid(AddrLin, phy_addr, pde_addr_phy, pde_Attribute, pte_Attribute);	
}

// used for DMA/PCI etc. mapping kernel lin addr(>kernelsize) to phyaddr
// 在公共内核页表映射一个页面到物理地址phy_addr
PUBLIC int kmapping_phy(u32 phy_addr) {
	u32 lin_addr;
	if(phy_addr >=  K_LIN2PHY(KernelLinMapBase)){// 高端地址，3G+phy_addr无法直接访问
		if(nr_kmapping_pages == KernelLinMapMaxPage) {
			disp_str("no free kmapping space");
			return -1;
		}
		lock_or_yield(&kmap_lock);
		int index = 0;
		while(index < KernelLinMapMaxPage && kmapping_pages[index] != 0)index++;
		kmapping_pages[index] = phy_addr;
		lin_addr = KernelLinMapBase + (index << PAGE_SHIFT);
		nr_kmapping_pages++;
		release(&kmap_lock);
	}else if(phy_addr >= kernel_size) {
		lin_addr = K_PHY2LIN(phy_addr); // 用户页物理地址，默认没有内核页表，需要写页表
	}else {
		return K_PHY2LIN(phy_addr); // 无需映射，可直接访问
	}
	lin_mapping_phy_nopid(lin_addr, 
						phy_addr, 
						kernel_pde_phy, 
						PG_P | PG_USS | PG_RWW,
						PG_P | PG_USS | PG_RWW);

	return lin_addr;
}

PUBLIC int kunmapping_phy(u32 phy_addr) {
	u32 lin_addr = K_PHY2LIN(phy_addr);
	if(phy_addr >=  K_LIN2PHY(KernelLinMapBase)){
		if(nr_kmapping_pages == 0) {
			disp_str("no kmapping now");
			return -1;
		}
		lock_or_yield(&kmap_lock);
		int index = 0;
		while(index < KernelLinMapMaxPage && kmapping_pages[index] != phy_addr)index++;
		if(index == KernelLinMapMaxPage) {
			disp_str("no phy in kmapping");
			release(&kmap_lock);
			return -1;
		}
		kmapping_pages[index] = 0;
		lin_addr = KernelLinMapBase + (index << PAGE_SHIFT);
		nr_kmapping_pages--;
		release(&kmap_lock);
	}else if(phy_addr < kernel_size) {
		return 0;
	}
	u32 pte_phy_addr = get_pte_phy_addr(kernel_pde_phy, lin_addr);
	write_page_pte(pte_phy_addr, lin_addr, 0, 0);
	refresh_page_cache();
	return 0;
}

/*======================================================================*
*                          clear_kernel_pagepte_low		add by visual 2016.5.12
*将内核低端页表清除
*======================================================================*/
void clear_kernel_pagepte_low()
{
	u32 page_num = *(u32 *)PageTblNumAddr;
	memset((void *)(K_PHY2LIN(KernelPageTblAddr)), 0, 4 * page_num);			 //从内核页目录中清除内核页目录项前8项
	memset((void *)(K_PHY2LIN(KernelPageTblAddr + num_4K)), 0, num_4K * page_num); //从内核页表中清除线性地址的低端映射关系
	refresh_page_cache();
}

/*======================================================================*
*						和页释放相关的函数
											added by mingxuan 2021-1-4
 *======================================================================*/

//清除页表项但不释放物理页
//added by mingxuan 2021-1-4
PUBLIC void clear_pte(u32 pid, u32 AddrLin)
{
	u32 pte_phy_addr = get_pte_phy_addr(get_pde_phy_addr(pid), AddrLin);
	write_page_pte(pte_phy_addr, AddrLin, 0, 0);
}

//清除页目录表项但不释放页表
//added by mingxuan 2021-1-4
PUBLIC void clear_pde(u32 pid, u32 AddrLin)
{
	u32 pde_phy_addr = get_pde_phy_addr(pid);
	write_page_pde(pde_phy_addr, AddrLin, 0, 0);
}

//释放物理页，并清除该物理页对应的页表项
//added by mingxuan 2021-1-4
PUBLIC void free_phypage(u32 pid, u32 AddrLin)
{
	ker_ufree_4k(pid,AddrLin);
}

//释放页表，并清除该页表对应的页目录表项
//added by mingxuan 2021-1-4
PUBLIC void free_pagetbl(u32 pid, u32 AddrLin)
{
	u32 pagetbl_phy_addr = get_pte_phy_addr(get_pde_phy_addr(pid), AddrLin);
	//do_kfree_4k(pagetbl_phy_addr);
	phy_kfree_4k(pagetbl_phy_addr); //modified by mingxuan 2021-8-16
	clear_pde(pid, AddrLin);
}

//释放页目录表
//added by mingxuan 2021-1-4
PUBLIC void free_pagedir(u32 pid)
{
	u32 pagedir_phy_addr = get_pde_phy_addr(pid);
	//do_kfree_4k(pagedir_phy_addr);
	phy_kfree_4k(pagedir_phy_addr); //modified by mingxuan 2021-8-16
	p_proc_current->task.cr3 = 0;
}

//added by mingxuan 2021-1-7
//PUBLIC u32 set_seg_base(pid, type)
PUBLIC u32 get_seg_base(u32 pid, u32 type) //modified by mingxuan 2021-8-17
{
	if (type == MEMMAP_TEXT)
		return proc_table[pid].task.memmap.text_lin_base;
	else if (type == MEMMAP_DATA)
		return proc_table[pid].task.memmap.data_lin_base;
	else if (type == MEMMAP_VPAGE)
		return proc_table[pid].task.memmap.vpage_lin_base;
	else if (type == MEMMAP_HEAP)
		return proc_table[pid].task.memmap.heap_lin_base;
	else if (type == MEMMAP_STACK)
		return proc_table[pid].task.memmap.stack_lin_base;
	else if (type == MEMMAP_ARG)
		return proc_table[pid].task.memmap.arg_lin_base;
	else if (type == MEMMAP_KERNEL)
		return proc_table[pid].task.memmap.kernel_lin_base;
	return 0;
}

//added by mingxuan 2021-1-7
//PUBLIC u32 set_seg_limit(pid, type)
PUBLIC u32 get_seg_limit(u32 pid, u32 type) //modified by mingxuan 2021-8-17
{
	if (type == MEMMAP_TEXT)
		return proc_table[pid].task.memmap.text_lin_limit;
	else if (type == MEMMAP_DATA)
		return proc_table[pid].task.memmap.data_lin_limit;
	else if (type == MEMMAP_VPAGE)
		return proc_table[pid].task.memmap.vpage_lin_limit;
	else if (type == MEMMAP_HEAP)
		return proc_table[pid].task.memmap.heap_lin_limit;
	else if (type == MEMMAP_STACK)
		return proc_table[pid].task.memmap.stack_lin_limit;
	else if (type == MEMMAP_ARG)
		return proc_table[pid].task.memmap.arg_lin_limit;
	else if (type == MEMMAP_KERNEL)
		return proc_table[pid].task.memmap.kernel_lin_limit;
	return 0;
}

//释放进程的某个段对应的所有物理页
//added by mingxuan 2021-1-7
PUBLIC void free_seg_phypage(u32 pid, u8 type)
{
	u32 addr_lin;
	u32 base, limit;

	//base = set_seg_base(pid, type);
	//limit = set_seg_limit(pid, type);
	//modified by mingxuan 2021-8-17
	base = get_seg_base(pid, type);
	limit = get_seg_limit(pid, type);

	//释放栈
	/*
	if(type == MEMMAP_STACK) //栈段是从高低址向低地址生长的，释放时与其他不同
	{
		//特别注意，limit的物理地址是取不到的，因为之前没有对limit所在的线性地址做映射,如何处理还需要再思考, mingxuan 2021-1-7
		u32 limit_phy_addr = get_page_phy_addr(pid, base - num_4K) - num_4K;//栈空间必须是连续的，不然会出错
		for (addr_lin = base - num_4K; addr_lin > limit; addr_lin -= num_4K)
		{
			free_phypage(pid, addr_lin);
		}
		//特别注意，limit的物理地址是取不到的，因为之前没有对limit所在的线性地址做映射
		//do_free_4k(limit_phy_addr); //栈空间必须是连续的，不然会出错
		phy_free_4k(limit_phy_addr); //modified by mingxuan 2021-8-14
	}
	*/
	//modified by mingxuan 2021-8-25
	if (type == MEMMAP_STACK) //栈段是从高低址向低地址生长的，释放时与其他不同
	{
		//特别注意，limit的物理地址是取不到的，因为之前没有对limit所在的线性地址做映射,如何处理还需要再思考, mingxuan 2021-1-7
		for (addr_lin = limit; addr_lin < UPPER_BOUND_4K(base); addr_lin += num_4K)
		{
			free_phypage(pid, addr_lin);
		}
	}

	else //释放其他段
	{
		for (addr_lin = base; addr_lin < UPPER_BOUND_4K(limit); addr_lin += num_4K)
		{
			free_phypage(pid, addr_lin);
		}
	}
}

//释放进程的全部物理页
//added by mingxuan 2021-1-6
PUBLIC void free_all_phypage(u32 pid)
{
	//释放代码段，text_hold为1就释放掉.为0不处理
	// if (proc_table[pid].task.info.text_hold == 1)
	// {
	// 全局页管理引入计数，不用再管代码持有问题
	free_seg_phypage(pid, MEMMAP_TEXT);
	// }
	free_seg_phypage(pid, MEMMAP_DATA);
	free_seg_phypage(pid, MEMMAP_VPAGE);
	free_seg_phypage(pid, MEMMAP_HEAP);
	free_seg_phypage(pid, MEMMAP_STACK);
	free_seg_phypage(pid, MEMMAP_ARG);

	return;
}

//释放进程的某个段对应的所有页表
//added by mingxuan 2021-1-7
PUBLIC void free_seg_pagetbl(u32 pid, u8 type)
{
	u32 addr_lin;
	u32 base, limit;

	//base = set_seg_base(pid, type);
	//limit = set_seg_limit(pid, type);
	//modified by mingxuan 2021-8-17
	base = get_seg_base(pid, type);
	limit = get_seg_limit(pid, type);

	//释放栈
	if (type == MEMMAP_STACK) //栈段是从高低址向低地址生长的，释放时与其他不同
	{
		for (addr_lin = limit; addr_lin < base; addr_lin += num_4M)
		{
			if (1 == pte_exist(get_pde_phy_addr(pid), addr_lin)) //解决重复释放的问题
			{
				free_pagetbl(pid, addr_lin);
			}
		}
	}
	else //释放其他段
	{
		for (addr_lin = base; addr_lin < limit; addr_lin += num_4M)
		{
			if (1 == pte_exist(get_pde_phy_addr(pid), addr_lin)) //解决重复释放的问题
			{
				free_pagetbl(pid, addr_lin);
			}
		}
	}
}

//释放进程的全部页表(不能释放内核页表)
//added by mingxuan 2021-1-6
PUBLIC void free_all_pagetbl(u32 pid)
{
	free_seg_pagetbl(pid, MEMMAP_TEXT);
	free_seg_pagetbl(pid, MEMMAP_DATA); //这里存在重复释放的问题
	free_seg_pagetbl(pid, MEMMAP_VPAGE);
	free_seg_pagetbl(pid, MEMMAP_HEAP);
	free_seg_pagetbl(pid, MEMMAP_STACK);
	free_seg_pagetbl(pid, MEMMAP_ARG);

	//释放SharePage //释放SharePage,若释放会报错

	//释放内核的映射 //不能释放内核页表
	//free_seg_pagetbl(pid, MEMMAP_KERNEL);
}

/*======================================================================*

*                          update_heap_limit		added by wang 2021-8-26
*
      更新核内堆指针，使用户态下和核心态下堆空间保持一致
*======================================================================*/
void update_heap_limit(u32 pid, int tag)

{
	if (tag == 1) //heap_limit加上4K
	{
		if (proc_table[pid].task.info.type == TYPE_PROCESS)
		{
			p_proc_current->task.memmap.heap_lin_limit += num_4K;
		}
		else
		{
			*(u32 *)(p_proc_current->task.memmap.heap_lin_limit) += num_4K;
		}
	}
	else if (tag == -1) //heap_limit减去4K
	{
		if (proc_table[pid].task.info.type == TYPE_PROCESS)
		{
			p_proc_current->task.memmap.heap_lin_limit -= num_4K;
		}
		else
		{
			*(u32 *)(p_proc_current->task.memmap.heap_lin_limit) -= num_4K;
		}
	}
}

/*======================================================================*

*                          get_heap_limit		added by wang 2021-8-26
*
*======================================================================*/
u32 get_heap_limit(u32 pid)
{
	if (proc_table[pid].task.info.type == TYPE_PROCESS)
	{
		return proc_table[pid].task.memmap.heap_lin_limit;
	}
	else
	{
		return *(u32 *)(proc_table[pid].task.memmap.heap_lin_limit);
	}
}

/*======================================================================*

*                          update_heap_ptr		added by mingxuan 2021-3-25
*
      更新核内堆指针，使用户态下和核心态下堆空间保持一致
*======================================================================*/
/* deleted by wang 2021.8.26
void update_heap_ptr(u32 vaddr,int tag)

{
    if(tag == 1)
    {
        if(vaddr + num_4K > p_proc_current->task.memmap.heap_lin_limit)

            p_proc_current->task.memmap.heap_lin_limit = vaddr + num_4K;

    }
    else if(tag == -1)
    {
        if(vaddr + num_4K == p_proc_current->task.memmap.heap_lin_limit)

            p_proc_current->task.memmap.heap_lin_limit -= num_4K;

    }

}
*/

/*======================================================================*

*                          get_pte		added by wang 2021-6-19
*
      get page table element
*======================================================================*/
u32 get_pte(u32 addrLin)
{
	u32 pte_phy_addr = get_pte_phy_addr(get_pde_phy_addr(p_proc_current->task.pid), addrLin);
	u32 pte_index = get_pte_index(addrLin);
	return (*((u32 *)K_PHY2LIN(pte_phy_addr) + pte_index));
}

/*======================================================================*

*                          get_pde		added by wang 2021-7-7
*
      get page directory element
	  主要用于测试内核页表建立函数SetupPaging的修改是否正确。
*======================================================================*/
int get_pde(u32 num)
{
	u32 i, pde_phy_addr;
	//pde_phy_addr=get_pde_phy_addr(p_proc_current->task.pid);
	pde_phy_addr = 0x100000;
	disp_str("pde_base_addr= ");
	disp_int(pde_phy_addr);
	disp_str("\n");

	for (i = 0; i < num; i++)
	{
		disp_int(*((u32 *)K_PHY2LIN(pde_phy_addr) + i));
		disp_str("\n");
	}
	return 0;
}
