/*********************************************************
*系统调用具体函数的实现
*
*
*
**********************************************************/
/*
#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "buddy.h"
*/
#include "../include/type.h"
#include "../include/const.h"
#include "../include/protect.h"
#include "../include/string.h"
#include "../include/proc.h"
#include "../include/global.h"
#include "../include/proto.h"
#include "../include/buddy.h"
#include "../include/kmalloc.h"
#include "../include/semaphore.h"

struct memfree *memarg = 0;



/*======================================================================*
                           sys_get_ticks		add by visual 2016.4.6
 *======================================================================*/
/* //deleted by mingxuan 2021-8-13
PUBLIC int sys_get_ticks()
{
	return ticks;
}
*/

PUBLIC int sys_get_ticks()
{
	return do_get_ticks();
}

PUBLIC int do_get_ticks()
{
	return kern_get_ticks();
}

//added by mingxuan 2021-8-14
PUBLIC int kern_get_ticks()
{
	return ticks;
}

/*======================================================================*
                           sys_get_pid		add by visual 2016.4.6
 *======================================================================*/
/* //deleted by mingxuan 2021-8-13
PUBLIC int sys_get_pid()
{
	return p_proc_current->task.pid;
}
*/
PUBLIC int sys_get_pid()
{
	return do_get_pid();
}

PUBLIC int do_get_pid()
{
	return kern_get_pid();
}

//added by mingxuan 2021-8-14
PUBLIC int kern_get_pid()
{
	return p_proc_current->task.pid;
}

/*======================================================================*
                           sys_kmalloc		add by visual 2016.4.6
 *======================================================================*/
/* //此函数已废弃不用 //deleted by mingxuan 2021-3-25
PUBLIC void* sys_kmalloc(int size)
{						//edit by visual 2015.5.9
	//此函数已废弃不用 //deleted by mingxuan 2021-3-25
	//return (void*)(do_kmalloc(size));
}
*/

/*======================================================================*
                           sys_kmalloc_4k		add by visual 2016.4.7
 *======================================================================*/
/* //此函数已废弃不用 //deleted by mingxuan 2021-3-25
PUBLIC void* sys_kmalloc_4k()
{	//此函数已废弃不用 //deleted by mingxuan 2021-3-25
	//return (void*)(do_kmalloc_4k());
}
*/

/*======================================================================*
                           sys_malloc		edit by visual 2016.5.4
 *======================================================================*/
/* //此函数已废弃不用 //deleted by mingxuan 2021-3-25
PUBLIC void* sys_malloc(int size)
{
	int vir_addr,phy_addr,AddrLin,pde_addr_phy,pte_addr_phy;
	vir_addr = vmalloc(size);

	for( AddrLin=vir_addr; AddrLin<vir_addr+size ; AddrLin += num_4B )//一个字节一个字节处理
	{
		lin_mapping_phy(	AddrLin,//线性地址					//add by visual 2016.5.9
							MAX_UNSIGNED_INT,//物理地址						//edit by visual 2016.5.19
							p_proc_current->task.pid,//进程pid				//edit by visual 2016.5.19
							PG_P  | PG_USU | PG_RWW,//页目录的属性位
							PG_P  | PG_USU | PG_RWW);//页表的属性位
	}
	return (void*)vir_addr;
}
*/

/*======================================================================*
                           sys_malloc_4k		edit by visual 2016.5.4
 *======================================================================*/
/*
PUBLIC void* sys_malloc_4k()
{
	int vir_addr,AddrLin,pde_addr_phy,pte_addr_phy,phy_addr;
	vir_addr = vmalloc(num_4K);

	for( AddrLin=vir_addr; AddrLin<vir_addr+num_4K ; AddrLin += num_4K )//一页一页处理(事实上只有一页，而且一定没有填写页表，页目录是否填写不确定)
	{
		lin_mapping_phy(	AddrLin,//线性地址					//add by visual 2016.5.9
							MAX_UNSIGNED_INT,//物理地址
							p_proc_current->task.pid,//进程pid					//edit by visual 2016.5.19
							PG_P  | PG_USU | PG_RWW,//页目录的属性位
							PG_P  | PG_USU | PG_RWW);//页表的属性位
	}
	return (void*)vir_addr;
}
*/
// modified by mingxuan 2021-3-25
/*
PUBLIC u32 sys_malloc_4k(u32 vaddr)
{


    if(vaddr==0)
        return p_proc_current->task.memmap.heap_lin_base;
    else
    {
        int AddrLin;

        for( AddrLin=vaddr; AddrLin<vaddr+num_4K ; AddrLin += num_4K )//一页一页处理(事实上只有一页，而且一定没有填写页表，页目录是否填写不确定)
        {
            lin_mapping_phy(	AddrLin,//线性地址					//add by visual 2016.5.9
                                MAX_UNSIGNED_INT,//物理地址
                                p_proc_current->task.pid,//进程pid					//edit by visual 2016.5.19
                                PG_P  | PG_USU | PG_RWW,//页目录的属性位
                                PG_P  | PG_USU | PG_RWW);//页表的属性位
        }

        update_heap_ptr(vaddr,1);
//        Scan_free_area(ubud);
        return 0;
    }
}
*/

/* //deleted by mingxuan 2021-8-14
// edited by wang 2021.8.10
PUBLIC u32 sys_malloc_4k()
{

	u32 AddrLin;

	AddrLin = p_proc_current->task.memmap.heap_lin_limit;

	//disp_int(p_proc_current->task.memmap.heap_lin_base);
	//disp_str("limit=");
	//disp_int(p_proc_current->task.memmap.heap_lin_limit);


	p_proc_current->task.memmap.heap_lin_limit += num_4K;

	lin_mapping_phy(AddrLin,				  //线性地址					//add by visual 2016.5.9
					MAX_UNSIGNED_INT,		  //物理地址
					p_proc_current->task.pid, //进程pid					//edit by visual 2016.5.19
					PG_P | PG_USU | PG_RWW,	  //页目录的属性位
					PG_P | PG_USU | PG_RWW);  //页表的属性位

	//update_heap_ptr(vaddr, 1);
	//        Scan_free_area(ubud);
	return AddrLin;
}
*/

//modified by mingxuan 2021-8-14
//PUBLIC u32 do_malloc_4k()
PUBLIC u32 kern_malloc_4k() //modified by mingxuan 2021-8-19
{

	u32 AddrLin;

	AddrLin = get_heap_limit(p_proc_current->task.pid);

	update_heap_limit(p_proc_current->task.pid, 1);

	//AddrLin = p_proc_current->task.memmap.heap_lin_limit;
	//AddrLin = *(u32 *)p_proc_current->task.memmap.heap_lin_limit;	//modified by mingxuan 2021-8-19
	//disp_int(p_proc_current->task.memmap.heap_lin_base);
	//disp_str("limit=");
	//disp_int(p_proc_current->task.memmap.heap_lin_limit);
	//p_proc_current->task.memmap.heap_lin_limit += num_4K;

	lin_mapping_phy(AddrLin,				  //线性地址					//add by visual 2016.5.9
					MAX_UNSIGNED_INT,		  //物理地址
					p_proc_current->task.pid, //进程pid					//edit by visual 2016.5.19
					PG_P | PG_USU | PG_RWW,	  //页目录的属性位
					PG_P | PG_USU | PG_RWW);  //页表的属性位

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

/*======================================================================*
                           sys_free		add by visual 2016.4.7
 *======================================================================*/
/* //此函数已废弃不用 //deleted by mingxuan 2021-3-25
PUBLIC int sys_free(void *arg)
{
	//memarg = (struct memfree *)arg;			 //deleted by mingxuan 2021-3-17
	//return do_free(memarg->addr,memarg->size); //deleted by mingxuan 2021-3-17

	//此函数已废弃不用 //deleted by mingxuan 2021-3-25
	//return(do_kfree(K_LIN2PHY((u32)arg)));	//modified by mingxuan 2021-3-17	//deleted by mingxuan 2021-3-25
}
*/

/*======================================================================*
                           sys_free_4k		edit by visual 2016.5.9
 *======================================================================*/
/*
PUBLIC int sys_free_4k(void* AddrLin)
{//线性地址可以不释放，但是页表映射关系必须清除！
	int phy_addr;				//add by visual 2016.5.9

	phy_addr = get_page_phy_addr(p_proc_current->task.pid,(int)AddrLin);//获取物理页的物理地址		//edit by visual 2016.5.19
	lin_mapping_phy(	(int)AddrLin,//线性地址
						phy_addr,//物理地址
						p_proc_current->task.pid,//进程pid			//edit by visual 2016.5.19
						PG_P  | PG_USU | PG_RWW,//页目录的属性位
						0  | PG_USU | PG_RWW);//页表的属性位
	return do_free_4k(phy_addr);
}
*/
//modified by mingxuan 2021-3-25
/*	//deleted by mingxuan 2021-8-14
PUBLIC int sys_free_4k(void* AddrLin)
{

    int phy_addr;

    phy_addr = get_page_phy_addr(p_proc_current->task.pid,(int)AddrLin);//获取物理页的物理地址

    clear_pte(p_proc_current->task.pid,AddrLin);

    update_heap_ptr(AddrLin,-1); //update heap pointer

    return do_free_4k(phy_addr);
}
*/

//modified by mingxuan 2021-8-14
PUBLIC int sys_free_4k(void *uesp)
{
	return do_free_4k(get_arg(uesp, 1));
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

	clear_pte(p_proc_current->task.pid, AddrLin);

	//    update_heap_ptr(AddrLin,-1); //update heap pointer
	update_heap_limit(p_proc_current->task.pid, -1); //update heap limit edited by wang 2021.8.26

	return phy_free_4k(phy_addr);
}

/*======================================================================*
*                          sys_fork		add by visual 2016.4.8
*sys_fork()放在了fork.c文件中
*======================================================================*/

/*======================================================================*
                           sys_pthread		add by visual 2016.4.11
*sys_pthread()放在了pthread.c文件中
 *======================================================================*/

/*======================================================================*
                           sys_udisp_int		add by visual 2016.5.16
用户用的打印函数
 *======================================================================*/
/*
PUBLIC void sys_udisp_int(int arg)
{
	disp_int(arg);
	return ;
}
*/
//modified by mingxuan 2021-8-13
PUBLIC void sys_udisp_int(void *uesp)
{
	do_udisp_int(get_arg(uesp, 1));
	return;
}

PUBLIC void do_udisp_int(int arg)
{
	disp_int(arg);
	return;
}

/*======================================================================*
                           sys_udisp_str		add by visual 2016.5.16
用户用的打印函数
 *======================================================================*/
/*
PUBLIC void sys_udisp_str(char *arg)
{
	disp_str(arg);
	return ;
}
*/
//modified by mingxuan 2021-8-13
PUBLIC void sys_udisp_str(void *uesp)
{
	do_udisp_str(get_arg(uesp, 1));
	return;
}

PUBLIC void do_udisp_str(char *arg)
{
	disp_str(arg);
	return;
}

/*======================================================================*
*                          sys_exec		被放在exec.c文件中
*======================================================================*/
/*======================================================================*
*                          sys_total_mem_size		added by wang 2021.8.21
*======================================================================*/

PUBLIC u32 sys_total_mem_size()
{
	u32 total_mem_size = 0;
	total_mem_size = kbud->current_mem_size + ubud->current_mem_size + kmem.current_mem_size;
	return total_mem_size;
}

#define TEST_FOR_SEMAPHORE
#ifdef TEST_FOR_SEMAPHORE
// #define TEST1
#define TEST2
// struct memfree *memarg = 0;
static int i = 0;
static int tmp = 0;
static int acc=0;
struct Semaphore print_sem,full,empty;
#define first_adder 1
#define second_adder 2
#define producer 3
#define consumer 4
#define MAXSIZE 5
#define EMPTY 0
int sum=0;
/*
*This function is used to ensure that 
*the first addition operation 
*can be completed completely
*without being interrupted by another
*added by cjj 2021-12-25
*/
int test1(void)
{
	int i=10;
	int tmp;
	while(i)
	{
		ksem_wait(&print_sem,1);//ensure add and print won't be intereupted
		tmp=sum;
		int j=1000000;
		while (j)
		{
			j--;
		}
		sum=tmp+1;
		i--;
	
	disp_str("sum1=");
	disp_int(sum);

	
		ksem_post(&print_sem,1);//Release lock and Exit conflict domain
	}

    return 0;
}
 /*
*This function is used to ensure that 
*the second addition operation 
*can be completed completely
*without being interrupted by another
*added by cjj 2021-12-25
*/
int test2(void)
{
	
    int i=10;
	int tmp;
    while(i)
    {
		ksem_wait(&print_sem,1);//ensure add and print won't be intereupted
		tmp=sum;
		int j=1000000;
		while (j)
		{
			j--;
		}
		sum=tmp+1;
		i--;
    // printf("sum2=%d\n",sum);
	disp_str("sum2=");
	disp_int(sum);

		ksem_post(&print_sem,1);//Release lock and Exit conflict domain
    }


    return 0;
}
/*
*This function is used to represent producers. 
*There are three producers in total
*/
int test_produce(int x)
{
	
    while(1)
    {
		int j =70000000;
		while (j)
		{
			j--;
		}
		
		ksem_wait(&full,1);
		ksem_wait(&print_sem,1);
		// printf("pth%d ",x);
		disp_str("pth");
		disp_int(x);
		disp_str("one product has been stored by");
		disp_int(x);
		disp_str("\n");
		// printf("one product has been stored by %d     \n",x);
		ksem_post(&print_sem,1);
		ksem_post(&empty,1);
    }
	while (1)
	{
		/* code */
	}
	
    return 0;
}
/*
*This function is used to represent consumer. 
*/
int test_consume(int x)
{

    while(1)
    {
		int j =10000000;
		while (j)
		{
			j--;
		}
		
#ifdef TEST1	
		int num=ksem_getvalue(&empty);

		if (ksem_trywait(&empty,num) !=-1)
		{
			ksem_wait(&print_sem,1);
			// printf("pth%d ",x);
			// printf("someone has bought %d product       \n",num);
			disp_str("pth");
			disp_int(x);
			disp_str("someone has bought ");
			disp_int(num);
			disp_str(" product\n");
			ksem_post(&print_sem,1);
			ksem_post(&full,num);
		}
#endif
#ifdef TEST2	
			ksem_wait(&empty,1);
			ksem_wait(&print_sem,1);
			disp_str("pth");
			disp_int(x);
			disp_str("someone has bought ");
			disp_int(1);
			disp_str(" product\n");
			ksem_post(&print_sem,1);
			ksem_post(&full,1);

#endif
    }

	
    return 0;
}
/*======================================================================*
*                          sys_test					added by cjj 2021.12.25	modified by Juan 2021.12.26
用于测试内核信号量的功能
*======================================================================*/

PUBLIC void sys_test(int function)
{
	do_test(function);
	return;
}
PUBLIC void do_test(int function)
{
	kern_test(function);
	return;
}

PUBLIC void kern_test(int function)
{
	
	if (!tmp)
	{
		tmp=1;
		ksem_init(&print_sem,1);					//Semaphore to ensure atomic operation
		ksem_init(&full,MAXSIZE);	//Semaphore used to judge whether the warehouse is full
		ksem_init(&empty,EMPTY);		//Semaphore used to judge whether the warehouse is empty
		// printf("1");
	}

	switch(function)
	{
		case	first_adder:    test1();
								break;

		case	second_adder:	test2();
								break;

		case 	producer:	  //	ksem_wait(print_sem,1);
								i++;
								test_produce(i);
								// ksem_post(print_sem,1);
								break;

		case 	consumer:	   //	ksem_wait(print_sem,1);
								i++; 
								test_consume(i);
								//ksem_post(print_sem,1);
								break;	

		default	: break;					
	}
	return ;

}
#endif /*TEST_FOR_SEMAPHORE*/