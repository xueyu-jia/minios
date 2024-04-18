
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "proc.h"
#include "clock.h"
#include "proto.h"
#include "console.h"
#include "buddy.h"
#include "ahci.h"
#include "dev.h"
#include "blame.h"
// #define GDBSTUB

#include "../gdbstub/gdbstub.h"

//added by lcy, 2023.10.22 
//与权限信息 RPL 存在耦合，不利于简化，不如这样： 20240418
#define common_cs (((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL)
#define common_ds (((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL)
#define common_es (((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL)
#define common_fs (((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL) 
#define common_ss (((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL)
#define common_gs (SELECTOR_KERNEL_GS & SA_RPL_MASK)

PRIVATE int initialize_processes(); //added by xw, 18/5/26
PRIVATE int initialize_cpus();		//added by xw, 18/6/2
PRIVATE void init_process(PROCESS *proc, char name[32], enum proc_stat stat, int pid, int is_rt, int priority_or_nice);//added by lcy, 2023.10.22
PRIVATE void init_reg(PROCESS *proc,u32 cs,u32 ds,u32 es,u32 fs,u32 ss,u32 gs,u32 eflags,u32 esp,u32 eip);//added by lcy, 2023.10.25

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{   
#ifdef GDBSTUB
	gdb_sys_init();
#endif
	int error;

	// kernel_initial = 1; //kernel is in initial state. added by xw, 18/5/31 
	// moved to cstart begin
	//zcr added(清屏)
	disp_pos = 0;
	for (int i = 0; i < 25; i++)
	{
		for (int j = 0; j < 80; j++)
		{
			disp_str(" ");
		}
	}
	disp_pos = 0;
    

	disp_str("-----Kernel Initialization Begins-----\n");

	//init();//内存管理模块的初始化  add by liang
	//buddy_init();	//modified by mingxuan 2021-3-8	//moved to kernel_main, mingxuan 2021-8-25

	sched_init();
	//initialize PCBs, added by xw, 18/5/26
	error = initialize_processes();
	if (error != 0)
		return error;

	//initialize CPUs, added by xw, 18/6/2
	error = initialize_cpus();
	if (error != 0)
		return error;

	k_reenter = 0; //record nest level of only interruption! it's different from Orange's.
		//usage modified by xw
	p_proc_current = cpu_table;

	/************************************************************************
	*device initialization
	added by xw, 18/6/4
	*************************************************************************/
	init_kb(); //added by mingxuan 2019-5-19

	AHCI_init();

	/* initialize hd-irq and hd rdwt queue */
	init_hd();

	/* initialize message queue */
	init_msgq(); //added by yingchi 2021.12.24

	init_devices();
	register_fs_types();
	/* initialize 8253 PIT */
	init_clock(); // read rtc init, 放在硬盘、键盘后面初始化减少误差
	/* enable interrupt, we should read information of some devices by interrupt.
	 * Note that you must have initialized all devices ready before you enable
	 * interrupt. added by xw
	 */
	enable_int();

	/***********************************************************************
	open hard disk and initialize file system
	coded by zcr on 2017.6.10. added by xw, 18/5/31
	************************************************************************/
	// init_fileop_table(); //delete by xiaofeng 2022-8-17

	// hd_open(0);
	// hd_open(1); //modified by mingxuan 2020-10-27
	init_open_hd();
	//modified by mingxuan 2020-10-27
	// hd_open(PRIMARY_SLAVE);
	// hd_open(SECONDARY_MASTER);
	// hd_open(SECONDARY_SLAVE);

	init_buffer(64);
	init_fs();
	// init_all_fat(SATA_BASE);
	//init_fs_fat();	//added by mingxuan 2019-5-17
	//init_vfs();	//added by mingxuan 2019-5-17	//deleted by mingxuan 2020-10-30

	// ksem_init(&proc_table_sem,1); 	//init PCB sem
	
	//初始状态确保hd_service放在实时队列第一个
	// in_rq(&proc_table[1]);
	// in_rq(&proc_table[0]);
	// in_rq(&proc_table[1]);
	// in_rq(&proc_table[2]);
	// in_rq(&proc_table[16]);
	// for(int pid=3; pid<NR_K_PCBS+1 ; pid++)
	// {
	// 	if(proc_table[pid].task.stat==READY)
	// 	{
	// 		in_rq(&proc_table[pid]);
	// 	}
	// }

	/*************************************************************************
	*第一个进程开始启动执行
	**************************************************************************/
	/* we don't want interrupt happens before processes run.
	 * added by xw, 18/5/31
	 */
	disable_int();

	disp_str("-----Processes Begin-----\n");

	/* linear address 0~8M will no longer be mapped to physical address 0~8M.
	 * note that disp_xx can't work after this function is invoked until processes runs.
	 * add by visual 2016.5.13; moved by xw, 18/5/30
	 * 清掉低端页表后 disp_xx不运行这个问题已经修复  modified by sundong 2023.3.8
	 */
	//clear_kernel_pagepte_low(); //delete by sundong 2023.3.8 因为kernel重新初始化页表的时候就删掉了低端页表

	// p_proc_current = proc_table;
	p_proc_current = &proc_table[PID_INIT];
	p_proc_current->task.cwd = vfs_root;
	cr3_ready=p_proc_current->task.cr3;

	//test_alloc_pages();      //test memory management functions    added by wang 2021.3.25
	//test_free_pages();
	//test_kmalloc();
	//test_kmalloc_kfree_over4k();
	//test_kfree();

	disp_str("main:total_mem_size=");
	disp_int(kern_total_mem_size());
	disp_str("\n");
	initlock(&video_mem_lock, "vmem");
	kernel_initial = 0; 
	//kernel initialization is done. added by xw, 18/5/31

	//test_kbud_mem_size();
	//test_kfree();

	#ifdef BLAME_STAT
		stat_init();// stat performance
	#endif
	restart_initial(); //modified by xw, 18/4/19
	while (1)
	{
	}
}

/*************************************************************************
return 0 if there is no error, or return -1.
added by xw, 18/6/2
***************************************************************************/
PRIVATE int initialize_cpus()
{
	//just use the fields of struct PCB in cpu_table, we needn't initialize
	//something at present.

	return 0;
}

/*************************************************************************
进程基本信息初始化（name、stat等）
added by lcy, 2023.10.22
***************************************************************************/
PRIVATE void init_process(PROCESS *proc, char name[32], enum proc_stat stat, int pid, int is_rt, int priority_or_nice){
		strcpy(proc->task.p_name, name); //名称
		proc->task.pid = pid;					   //pid
		proc->task.stat = stat;				   //初始化状态 -1表示未初始化
		proc->task.ticks = proc->task.priority = 1;	//时间片和优先级
		//proc->task.regs.eip = eip;
		
		//CFS
		proc->task.is_rt=is_rt;
		if(is_rt) {
			proc->task.rt_priority=priority_or_nice;
		}else {
			proc->task.nice=priority_or_nice;
			proc->task.weight=nice_to_weight[proc->task.nice+20];
		}
		proc->task.vruntime=0;
		proc->task.sum_cpu_use=0;
		proc->task.cpu_use=0;

}

PRIVATE void init_reg(PROCESS *proc,u32 cs,u32 ds,u32 es,u32 fs,u32 ss,u32 gs,u32 eflags,u32 esp,u32 eip){
		proc->task.esp_save_int->cs = cs;
		proc->task.esp_save_int->ds = ds;
		proc->task.esp_save_int->es = es;
		proc->task.esp_save_int->fs = fs;
		proc->task.esp_save_int->ss = ss;
		proc->task.esp_save_int->gs = gs;
		proc->task.esp_save_int->eflags =eflags;    /* IF=1, IOPL=1 */
		proc->task.esp_save_int->esp = esp;
		proc->task.esp_save_int->eip = eip;
}

// [start,end) 地址段分配页表，如果不存在分配物理页并填写页表，否则忽略 固定地址start<end
PRIVATE void init_proc_page_addr(u32 low, u32 high, int pid, u32 attr){
	u32 addr;
	int err_temp;
	for (addr = low; addr < UPPER_BOUND_4K(high); addr += num_4K)
	{ 
		err_temp=ker_umalloc_4k(addr,pid,attr);          //edited by wang 2021.8.27

		if (err_temp != 0)
		{
			disp_color_str("kernel_main Error:lin_mapping_phy", 0x74);
			return ;
		}
	}
}

// 调用此函数分配页表
PRIVATE void init_proc_pages(PROCESS* p_proc){
	int pid = p_proc->task.pid;
	if (0 != init_proc_page(pid))
	{
		disp_color_str("kernel_main Error:init_proc_page", 0x74);
		return ;
	}
	p_proc->task.memmap.heap_lin_base = HeapLinBase;
	p_proc->task.memmap.heap_lin_limit = HeapLinBase; //堆的界限将会一直动态变化

	p_proc->task.memmap.stack_child_limit = StackLinLimitMAX; //add by visual 2016.5.27
	p_proc->task.memmap.stack_lin_base = StackLinBase;
	p_proc->task.memmap.stack_lin_limit = StackLinBase - 0x4000; //栈的界限将会一直动态变化，目前赋值为16k，这个值会根据esp的位置进行调整，目前初始化为16K大小
	p_proc->task.memmap.arg_lin_base = ArgLinBase;						 //参数内存基址
	p_proc->task.memmap.arg_lin_limit = ArgLinLimitMAX;
	p_proc->task.memmap.kernel_lin_base = KernelLinBase;
	p_proc->task.memmap.kernel_lin_limit = KernelLinBase + kernel_size;
	init_proc_page_addr(p_proc->task.memmap.stack_lin_limit, StackLinBase, pid ,PG_P | PG_USU | PG_RWW);
	init_proc_page_addr(p_proc->task.memmap.arg_lin_base, p_proc->task.memmap.arg_lin_limit, pid ,PG_P | PG_USU | PG_RWW);
}

PRIVATE void init_proc_ldt_regs(PROCESS* p_proc, u16 ldt_sel, u32 rpl, u32 entry){
	u8 privilege = rpl;
	u32 eflags = (rpl != RPL_USER)? 0x1202 : 0x202;
	p_proc->task.ldt_sel = ldt_sel;
	memcpy(&p_proc->task.ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
	p_proc->task.ldts[0].attr1 = DA_C | privilege << 5;
	memcpy(&p_proc->task.ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
	p_proc->task.ldts[1].attr1 = DA_DRW | privilege << 5;
	char* p_regs;
	p_regs = (char *)(p_proc + 1);
	p_regs -= P_STACKTOP;
	////初始化内核栈
	/***************some field about process switch****************************/
	p_proc->task.esp_save_int = (STACK_FRAME*)p_regs; //initialize esp_save_int, added by xw, 17/12/11    //changed by lcy, 2023.10.24
	init_reg(p_proc,	common_cs|rpl,	common_ds|rpl,	common_es|rpl,
		common_fs|rpl,	common_ss|rpl,	common_gs|rpl,	eflags,	(u32)StackLinBase, entry);

	//p_proc->task.save_type = 1;
	p_proc->task.esp_save_context = p_regs - 10 * 4; //when the process is chosen to run for the first time,
														 //sched() will fetch value from esp_save_context
	*(u32 *)(p_regs - 4) = (u32)(restart_restore); //initialize EIP in the context, so the process can
												 //start run. added by xw, 18/4/18
	*(u32 *)(p_regs - 8) = 0x1202;
}

/*************************************************************************
进程初始化部分
return 0 if there is no error, or return -1.
moved from kernel_main() by xw, 18/5/26
***************************************************************************/
PRIVATE int initialize_processes()
{
	TASK *p_task = task_table;
	PROCESS *p_proc = proc_table;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	// char *p_regs;		//point to registers in the new kernel stack, added by xw, 17/12/11
	// task_f eip_context; //a funtion pointer, added by xw, 18/4/18
	/*************************************************************************
	*进程初始化部分 	edit by visual 2016.5.4
	***************************************************************************/
	int pid;
	// u32 AddrLin, pte_addr_phy_temp, addr_phy_temp, err_temp; //edit by visual 2016.5.9
	
	/* set common fields in PCB. added by xw, 18/5/25 */
	p_proc = proc_table;
	for (pid = 0; pid < NR_PCBS; pid++)
	{
		// get entry, name, privilege;
		u32 entry = NULL;
		char* name;
		int ready = 0;
		int rpl = RPL_USER;
		int is_rt = false;
		int rtpriority_or_nice = 0;
		if(pid < NR_TASKS){
			entry = (u32)p_task->initial_eip;
			name = p_task->name;
			ready = p_task->ready;
			rpl = p_task->rpl;
			is_rt = p_task->rt;
			rtpriority_or_nice = p_task->priority_nice;
			p_task++;
		}else if(pid == PID_INIT){
			entry = (u32)initial;
			name = "initial";
			rpl = RPL_TASK;
			ready = 1;
		}else if(pid < NR_K_PCBS){
			name = "TASK";
		}else{
			name = "USER";
		}
		//init operations
		memset(p_proc,0,sizeof(PROCESS));//by qianglong
		init_proc_ldt_regs(p_proc, selector_ldt, rpl, entry);
		if(entry){
			init_process(p_proc, name, (ready ? READY: SLEEPING), pid, is_rt, rtpriority_or_nice);
			init_proc_pages(p_proc);
		}else{
			init_process(p_proc, name, FREE, pid, -1, 0);
		}
		if(ready) {
			in_rq(p_proc);
		}
		for (int j = 0; j < NR_FILES; j++)
		{
			p_proc->task.filp[j] = 0;
		}
		p_proc++;
		selector_ldt += 1 << 3;
	}
	p_proc = &proc_table[PID_INIT];
	/*************************进程树信息初始化***************************************/
	p_proc->task.info.type = TYPE_PROCESS; //当前是进程还是线程
	p_proc->task.info.real_ppid = -1;	   //亲父进程，创建它的那个进程
	p_proc->task.info.ppid = -1;		   //当前父进程
	p_proc->task.info.child_p_num = 0;	   //子进程数量
	// p_proc->task.info.child_process[NR_CHILD_MAX];//子进程列表
	p_proc->task.info.child_t_num = 0; //子线程数量
	// p_proc->task.info.child_thread[NR_CHILD_MAX];//子线程列表
	// p_proc->task.memmap.text_lin_base = 0;								 //initial这些段的数据并不清楚，在变身init的时候才在中赋新值
	// p_proc->task.memmap.text_lin_limit = 0;								 //initial这些段的数据并不清楚，在变身init的时候才在exec中赋新值
	// p_proc->task.memmap.data_lin_base = 0;								 //initial这些段的数据并不清楚，在变身init的时候才在exec中赋新值
	// p_proc->task.memmap.data_lin_limit = 0;								 //initial这些段的数据并不清楚，在变身init的时候才在exec中赋新值
	p_proc->task.memmap.vpage_lin_base = VpageLinBase;					 //保留内存基址
	p_proc->task.memmap.vpage_lin_limit = VpageLinBase;					 //保留内存界限

	return 0;

}

