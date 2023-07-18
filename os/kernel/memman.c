//#include "memman.h"
//#include "buddy.h"
#include "../include/buddy.h" //added by mingxuan 2021-3-8
#include "../include/shm.h"
/*
u32 MemInfo[256] = {0};			//存放FMIBuff后1k内容
void init()
{
    int i;
    memcpy(MemInfo,(u32 *)FMIBuff,1024);		//复制内存
    for(i=0;i<=MemInfo[0];i++)
    {
        disp_int(MemInfo[i]);
        disp_str("  ");
    }

}

*/

/* // deleted by mingxuan 2021-3-8
u32 MemInfo[256] = {0};			//存放FMIBuff后1k内容
struct MEMMAN s_memman;
struct MEMMAN *memman = &s_memman;//(struct MEMMAN *) MEMMAN_ADDR;

void memman_init(struct MEMMAN *man);
PUBLIC u32 memman_alloc(struct MEMMAN *man,u32 size);
PUBLIC u32 memman_kalloc(struct MEMMAN *man,u32 size);
PUBLIC u32 memman_alloc_4k(struct MEMMAN *man);
PUBLIC u32 memman_kalloc_4k(struct MEMMAN *man);
PUBLIC u32 memman_free(struct MEMMAN *man, u32 addr, u32 size);
PUBLIC void disp_free();
u32 memman_total(struct MEMMAN *man);

void init()	//初始化
{
	u32 memstart = MEMSTART;			//4M 开始初始化
	u32 i,j;

	memcpy(MemInfo,(u32 *)FMIBuff,1024);		//复制内存

	memman_init(memman);				//初始化memman中frees,maxfrees,lostsize,losts

	for(i = 1; i <= MemInfo[0]; i++)
	{
		if(MemInfo[i] < memstart)continue;	//4M 之后开始free
		memman_free(memman,memstart,MemInfo[i] - memstart);	//free每一段可用内存
		memstart = MemInfo[i] + 0x1000;	//memtest_sub(start,end)中每4KB检测一次
	}

	for(i = 0; i < memman->frees; i++)
	{//6M处分开，4～6M为kmalloc_4k使用，6～8M为kmalloc使用
		if((memman->free[i].addr <= KWALL)&&(memman->free[i].addr + memman->free[i].size > KWALL)){
			if(memman->free[i].addr == KWALL)break;
			else{

				for(j = memman->frees; j>i+1; j--)
				{	//i之后向后一位
					memman->free[j] = memman->free[j-1];
				}
				memman->frees++;
				if(memman->maxfrees < memman->frees){	//更新man->maxfrees
					memman->maxfrees = memman->frees;
				}
				memman->free[i+1].addr = KWALL;
				memman->free[i+1].size = memman->free[i].addr + memman->free[i].size - KWALL;
				memman->free[i].size = KWALL - 0x1000 - memman->free[i].addr;
				break;
			}
		}
	}
	for(i = 0; i < memman->frees; i++)
	{//8M处分开，4～8M为kmalloc使用，8～32M为malloc使用
		if((memman->free[i].addr <= WALL)&&(memman->free[i].addr + memman->free[i].size > WALL)){
			if(memman->free[i].addr == WALL)break;
			else{

				for(j = memman->frees; j>i+1; j--)
				{	//i之后向后一位
					memman->free[j] = memman->free[j-1];
				}
				memman->frees++;
				if(memman->maxfrees < memman->frees){	//更新man->maxfrees
					memman->maxfrees = memman->frees;
				}
				memman->free[i+1].addr = WALL;
				memman->free[i+1].size = memman->free[i].addr + memman->free[i].size - WALL;
				memman->free[i].size = WALL - 0x1000 - memman->free[i].addr;
				break;
			}
		}
	}

	for(i = 0; i < memman->frees; i++)
	{//16M处分开，8～16M为malloc使用，16～32M为malloc_4k使用
		if((memman->free[i].addr <= UWALL)&&(memman->free[i].addr + memman->free[i].size > UWALL)){
			if(memman->free[i].addr == UWALL)break;
			else{

				for(j = memman->frees; j>i+1; j--)
				{	//i之后向后一位
					memman->free[j] = memman->free[j-1];
				}
				memman->frees++;
				if(memman->maxfrees < memman->frees){	//更新man->maxfrees
					memman->maxfrees = memman->frees;
				}
				memman->free[i+1].addr = UWALL;
				memman->free[i+1].size = memman->free[i].addr + memman->free[i].size - UWALL;
				memman->free[i].size = UWALL - 0x1000 - memman->free[i].addr;
				break;
			}
		}
	}


	//modified by xw, 18/6/18
	// disp_str("**********");
	disp_str("Memory Available:");
	disp_int(memman_total(memman));	//显示初始总容量
	disp_str("\n");
	// disp_str("**********\n");
	//~xw

	return;

}	//于kernel_main()中调用，进行初始化

void memman_init(struct MEMMAN *man)
{	//memman基本信息初始化
	man->frees = 0;
	man->maxfrees = 0;
	man->lostsize = 0;
	man->losts = 0;
	return;
}

PUBLIC u32 memman_alloc(struct MEMMAN *man,u32 size)
{	//分配
	u32 i,a;
	for(i=0; i<man->frees; i++)
	{
		if((man->free[i].addr >= WALL)&&(man->free[i].addr + size < UWALL)){	//8M到16M
			if(man->free[i].size >= size){
				a = man->free[i].addr;
				man->free[i].addr += size;
				man->free[i].size -= size;
				if(man->free[i].size == 0){
					man->frees--;
					for(; i<man->frees; i++)
					{
						man->free[i] = man->free[i+1];
					}
				}
				return a;
			}
		}
	}
	return -1;
}

PUBLIC u32 memman_kalloc(struct MEMMAN *man,u32 size)
{	//分配
	u32 i,a;
	for(i=0; i<man->frees; i++)
	{
		if((man->free[i].addr >= KWALL)&&(man->free[i].addr + size < WALL)){	//4M到8M
			if(man->free[i].size >= size){
				a = man->free[i].addr;
				man->free[i].addr += size;
				man->free[i].size -= size;
				if(man->free[i].size == 0){
					man->frees--;
					for(; i<man->frees; i++)
					{
						man->free[i] = man->free[i+1];
					}
				}
				return a;
			}
		}

	}
	return -1;
}

PUBLIC u32 memman_alloc_4k(struct MEMMAN *man)
{	//分配
	u32 i,a;
	u32 size = 0x1000;
	for(i=0; i<man->frees; i++)
	{
		if((man->free[i].addr >= UWALL)&&(man->free[i].addr + size < MEMEND)){	//16M到32M
			if(man->free[i].size >= size){
				a = man->free[i].addr;
				man->free[i].addr += size;
				man->free[i].size -= size;
				if(man->free[i].size == 0){
					man->frees--;
					for(; i<man->frees; i++)
					{
						man->free[i] = man->free[i+1];
					}
				}
				return a;
			}
		}
	}
	return -1;
}

PUBLIC u32 memman_kalloc_4k(struct MEMMAN *man)
{	//分配
	u32 i,a;
	u32 size = 0x1000;
	for(i=0; i<man->frees; i++)
	{
		if((man->free[i].addr >= MEMSTART)&&(man->free[i].addr + size < KWALL)){	//4M到6M
			if(man->free[i].size >= size){
				a = man->free[i].addr;
				man->free[i].addr += size;
				man->free[i].size -= size;
				if(man->free[i].size == 0){
					man->frees--;
					for(; i<man->frees; i++)
					{
						man->free[i] = man->free[i+1];
					}
				}
				return a;
			}
		}
	}
	return -1;
}

PUBLIC u32 memman_free(struct MEMMAN *man, u32 addr, u32 size)
{	//释放
	int i,j;

	if(size == 0)return 0;	//初始化时，防止有连续坏块

	for(i=0; i<man->frees; i++)
	{
		if(man->free[i].addr > addr){
			break;
		}
	}	//man->free[i-1].addr < addr <man->free[i].addr

	if(i > 0){	//前面有可分配内存
		if(man->free[i-1].addr + man->free[i-1].size == addr){	//与前面相邻
			man->free[i-1].size += size;
			if(i < man->frees){	//后面有可分配内存
				if(addr + size == man->free[i].addr){	//同时与后面相邻
					man->free[i-1].size += man->free[i].size;
					man->frees--;
					for(; i<man->frees; i++)
					{
						man->free[i] = man->free[i+1];	//结构体赋值，i之后向前一位
					}
				}
			}
			return 0;
		}	//与前面不相邻
	}	//前面无可分配内存

	if(i < man->frees){	//后面有可分配内存
		if(addr + size == man->free[i].addr){	//与后面相邻
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0;
		}
	}

	if(man->frees < MEMMAN_FREES){	//数组未满
		for(j = man->frees; j>i; j--)
		{	//i之后向后一位
			man->free[j] = man->free[j-1];
		}
		man->frees++;
		if(man->maxfrees < man->frees){	//更新man->maxfrees
			man->maxfrees = man->frees;
		}
		man->free[i].addr = addr;	//插入
		man->free[i].size = size;
		return 0;
	}

	man->losts++;	//free失败
	man->lostsize += size;
	return -1;
}

PUBLIC u32 memman_free_4k(struct MEMMAN *man, u32 addr)
{
	return memman_free(man, addr, 0x1000);
}



PUBLIC u32 do_malloc(u32 size)
{
	return memman_alloc(memman,size);
}

PUBLIC u32 do_kmalloc(u32 size)
{
	return memman_kalloc(memman,size);
}

PUBLIC u32 do_malloc_4k()
{
	return memman_alloc_4k(memman);
}

PUBLIC u32 do_kmalloc_4k()
{
	return memman_kalloc_4k(memman);
}

PUBLIC u32 do_free(u32 addr,u32 size)
{
	return memman_free(memman,addr,size);
}

PUBLIC u32 do_free_4k(u32 addr)
{
	return memman_free_4k(memman,addr);
}

PUBLIC void disp_free()
{	//打印空闲内存块信息
	int i;
	for(i = 0; i < memman->frees; i++)
	{
		disp_int(memman->free[i].addr);
		disp_str("#");
		disp_int(memman->free[i].size);
		disp_str("###");
	}
}

u32 memman_total(struct MEMMAN *man)
{	//free总容量
	u32 i,t=0;
	for(i=0; i<man->frees; i++){
		t += man->free[i].size;
	}
	return t;
}

PUBLIC void memman_test()
{	//测试
	u32 *p1 = 0;
	u32 *p2 = 0;
	u32 *p3 = 0;
	u32 *p4 = 0;
	u32 *p  = 0;

	p1 = (u32 *)do_malloc(4);
	if(-1 != (u32)p1){	//打印p1，当前空闲内存信息，p1所指内存的内容
		disp_str("START");
		disp_int((u32)p1);
		//disp_free();
		*p1 = TEST;
		disp_int(*p1);
		disp_str("END");
	}

	p2 = (u32 *)do_kmalloc(4);
	if(-1 != (u32)p2){
		disp_str("START");
		disp_int((u32)p2);
		//disp_free();
		*p2 = TEST;
		disp_int(*p2);
		disp_str("END");
	}

	do_free((u32)p1,4);
	do_free((u32)p2,4);

	p3 = (u32 *)do_malloc_4k();
	if(-1 != (u32)p3){
		disp_str("START");
		disp_int((u32)p3);
		//disp_free();
		*p3 = TEST;
		disp_int(*p3);
		p = p3 + 2044;
		*p = 0x22334455;
		disp_int(*p);
		p += 2048;
		*p = 0x33445566;
		disp_int(*p);
		disp_str("END");
	}

	p4 = (u32 *)do_kmalloc_4k(4);
	if(-1 != (u32)p4){
		disp_str("START");
		disp_int((u32)p4);
		//disp_free();
		*p4 = TEST;
		disp_int(*p4);
		p = p4 + 2044;
		*p = 0x22334455;
		disp_int(*p);
		p += 2048;
		*p = 0x33445566;
		disp_int(*p);
		disp_str("END");
	}

	do_free_4k((u32)p3);
	do_free_4k((u32)p4);

	disp_str("START");
	disp_free();
	disp_str("END");

	return;
}
*/

//modified by mingxuan 2021-3-8

//extern buddy *kbud;
//extern buddy *ubud;
//extern free_mem *kfreem;
//extern free_mem *ufreem;

//在内核中直接使用,mingxuan 2021-3-25
//edited by wang 2021.6.8
/*
PUBLIC u32 do_kmalloc(u32 size) //有int型参数size，从内核线性地址空间申请一段大小为size的内存
{
	if (size < num_4K)
		return _kmalloc(size);
	else
		return kmalloc_over4k(size);
}
*/
//modified by mingxuan 2021-8-16
PUBLIC u32 phy_kmalloc(u32 size) //有int型参数size，从内核线性地址空间申请一段大小为size的内存
{
	if (size <= (1 << MAX_BUFF_ORDER))
		return kmalloc(size);
	else
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
	void *p = K_PHY2LIN(phy_kmalloc(size));
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
    int res = pfn_to_phy(page_to_pfn(page));
	//disp_str("m");
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

	//disp_str("f");
    page *page = pfn_to_page(phy_to_pfn(phy_addr));
	return free_pages(kbud, page, 0);
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
    int res = pfn_to_phy(page_to_pfn(page));
	if (res == 0)
		disp_color_str("phy_malloc_4k:alloc_pages Error,no memory", 0x74);
	return res;
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
	page *page = pfn_to_page(phy_to_pfn(phy_addr));
	return free_pages(ubud, page, 0);
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
PUBLIC int ker_umalloc_4k(u32 AddrLin, u32 pid, u32 pte_attribute)
{

	//不用判AddrLin是否4K对齐，lin_mapping_phy会为非4k对齐的线性地址分配物理页

	if (AddrLin > KernelLinBase)
	{
		disp_color_str("ker_umalloc_4k: address error ", 0x74);
		return -1;
	}

	return lin_mapping_phy(AddrLin, MAX_UNSIGNED_INT, pid, PG_P | PG_USU | PG_RWW, pte_attribute);
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
