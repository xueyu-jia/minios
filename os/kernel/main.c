
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <kernel/type.h>
#include <kernel/const.h>
#include <kernel/proc.h>
#include <kernel/clock.h>
#include <kernel/proto.h>
#include <kernel/console.h>
#include <kernel/buddy.h>
#include <kernel/ahci.h>
#include <kernel/dev.h>
#include <kernel/blame.h>
#include <kernel/mmap.h>
#include <kernel/pagetable.h>
#include <kernel/shm.h>
#include <kernel/hd.h>
#include <kernel/keyboard.h>
#include <kernel/devfs.h>
#ifndef OPT_DISP_SERIAL
// #define GDBSTUB
#endif
#include "../gdbstub/gdbstub.h"

//added by lcy, 2023.10.22
//与权限信息 RPL 存在耦合，不利于简化，不如这样： 20240418

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
	//zcr added(清屏)
	disp_pos = 0;
	for (int i = 0; i < 25; i++){
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
	// p_proc_current = cpu_table;

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

	/* init shm*/
	init_shm();
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


	init_buffer(64);
	init_fs(SATA_BASE);

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

	p_proc_current = &proc_table[PID_INIT];
	p_proc_current->task.cwd = vfs_root;
	cr3_ready=p_proc_current->task.cr3;


	disp_str("main:total_mem_size=");
	disp_int(kern_total_mem_size());
	disp_str("\n");
	init_ttys();
	initlock(&video_mem_lock, "vmem");
	kernel_initial = 0;
	//kernel initialization is done. added by xw, 18/5/31

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

// /*************************************************************************
// 进程基本信息初始化（name、stat等）
// added by lcy, 2023.10.22
// ***************************************************************************/
// PRIVATE void init_process(PROCESS *proc, char name[32], enum proc_stat stat, int pid, int is_rt, int priority_or_nice){
// 		strcpy(proc->task.p_name, name); //名称
// 		proc->task.pid = pid;					   //pid
// 		proc->task.stat = stat;				   //初始化状态 -1表示未初始化
// 		proc->task.ticks = proc->task.priority = 1;	//时间片和优先级
// 		//proc->task.regs.eip = eip;

// 		//CFS
// 		proc->task.is_rt=is_rt;
// 		if(is_rt) {
// 			proc->task.rt_priority=priority_or_nice;
// 		}else {
// 			proc->task.nice=priority_or_nice;
// 			proc->task.weight=nice_to_weight[proc->task.nice+20];
// 		}
// 		proc->task.vruntime=0;
// 		proc->task.sum_cpu_use=0;
// 		proc->task.cpu_use=0;

// }


// // 调用此函数分配页表
// PRIVATE void init_proc_pages(PROCESS* p_proc){
// 	int pid = p_proc->task.pid;
// 	if (0 != init_proc_page(pid))
// 	{
// 		disp_color_str("kernel_main Error:init_proc_page", 0x74);
// 		return ;
// 	}
// 	p_proc->task.memmap.heap_lin_base = HeapLinBase;
// 	p_proc->task.memmap.heap_lin_limit = HeapLinBase; //堆的界限将会一直动态变化

// 	p_proc->task.memmap.stack_child_limit = StackLinLimitMAX; //add by visual 2016.5.27
// 	p_proc->task.memmap.stack_lin_base = StackLinBase;
// 	p_proc->task.memmap.stack_lin_limit = StackLinBase - 0x4000; //栈的界限将会一直动态变化，目前赋值为16k，这个值会根据esp的位置进行调整，目前初始化为16K大小
// 	p_proc->task.memmap.arg_lin_base = ArgLinBase;						 //参数内存基址
// 	p_proc->task.memmap.arg_lin_limit = ArgLinLimitMAX;
// 	p_proc->task.memmap.kernel_lin_base = KernelLinBase;
// 	p_proc->task.memmap.kernel_lin_limit = KernelLinBase + kernel_size;
// 	list_init(&p_proc->task.memmap.vma_map);
// 	init_mem_page(&p_proc->task.memmap.anon_pages, MEMPAGE_AUTO);
// 	init_proc_page_addr(p_proc->task.memmap.stack_lin_limit, p_proc->task.memmap.stack_lin_base, p_proc);
// 	init_proc_page_addr(p_proc->task.memmap.arg_lin_base, p_proc->task.memmap.arg_lin_limit, p_proc);
// 	// init_proc_page_addr(p_proc->task.memmap.stack_lin_limit, StackLinBase, pid ,PG_P | PG_USU | PG_RWW);
// 	// init_proc_page_addr(p_proc->task.memmap.arg_lin_base, p_proc->task.memmap.arg_lin_limit, pid ,PG_P | PG_USU | PG_RWW);
// }

PRIVATE void init_proc_ldt_regs(PROCESS* p_proc, u32 rpl, u32 entry){
	STACK_FRAME* p_regs = proc_kstacktop(p_proc);
	////初始化内核栈
	/***************some field about process switch****************************/
	proc_init_ldt_kstack(p_proc, rpl);
	proc_kstacktop(p_proc)->eip = entry;
	proc_kstacktop(p_proc)->esp = p_proc->task.memmap.stack_lin_base;
	//p_proc->task.save_type = 1;
	proc_init_context(p_proc);
}

// /*************************************************************************
// 进程初始化部分
// return 0 if there is no error, or return -1.
// moved from kernel_main() by xw, 18/5/26
// ***************************************************************************/
// PRIVATE int initialize_processes()
// {
// 	TASK *p_task = task_table;
// 	PROCESS *p_proc = proc_table;
// 	// char *p_regs;		//point to registers in the new kernel stack, added by xw, 17/12/11
// 	// task_f eip_context; //a funtion pointer, added by xw, 18/4/18
// 	/*************************************************************************
// 	*进程初始化部分 	edit by visual 2016.5.4
// 	***************************************************************************/
// 	int pid;
// 	// u32 AddrLin, pte_addr_phy_temp, addr_phy_temp, err_temp; //edit by visual 2016.5.9

// 	/* set common fields in PCB. added by xw, 18/5/25 */
// 	p_proc = proc_table;
// 	for (pid = 0; pid < NR_PCBS; pid++)
// 	{
// 		// get entry, name, privilege;
// 		u32 entry = NULL;
// 		char* name;
// 		int ready = 0;
// 		int rpl = RPL_USER;
// 		int is_rt = false;
// 		int rtpriority_or_nice = 0;
// 		if(pid < NR_TASKS){
// 			entry = (u32)p_task->initial_eip;
// 			name = p_task->name;
// 			ready = p_task->ready;
// 			rpl = p_task->rpl;
// 			is_rt = p_task->rt;
// 			rtpriority_or_nice = p_task->priority_nice;
// 			p_task++;
// 		}else if(pid == PID_INIT){
// 			entry = (u32)initial;
// 			name = "initial";
// 			rpl = RPL_TASK;
// 			ready = 1;
// 		}else if(pid < NR_K_PCBS){
// 			name = "TASK";
// 		}else{
// 			name = "USER";
// 		}
// 		//init operations
// 		memset(p_proc,0,sizeof(PROCESS));//by qianglong
// 		if(entry){
// 			init_process(p_proc, name, (ready ? READY: SLEEPING), pid, is_rt, rtpriority_or_nice);
// 			init_proc_pages(p_proc);
// 			init_proc_ldt_regs(p_proc, rpl, entry);
// 		}else{
// 			init_process(p_proc, name, FREE, pid, 0, 0);
// 		}
// 		if(ready) {
// 			in_rq(p_proc);
// 		}
// 		for (int j = 0; j < NR_FILES; j++)
// 		{
// 			p_proc->task.filp[j] = 0;
// 		}
// 		p_proc++;
// 	}
// 	p_proc = &proc_table[PID_INIT];
// 	/*************************进程树信息初始化***************************************/
// 	p_proc->task.tree_info.type = TYPE_PROCESS; //当前是进程还是线程
// 	p_proc->task.tree_info.real_ppid = -1;	   //亲父进程，创建它的那个进程
// 	p_proc->task.tree_info.ppid = -1;		   //当前父进程
// 	p_proc->task.tree_info.child_p_num = 0;	   //子进程数量
// 	// p_proc->task.tree_info.child_process[NR_CHILD_MAX];//子进程列表
// 	p_proc->task.tree_info.child_t_num = 0; //子线程数量
// 	// p_proc->task.tree_info.child_thread[NR_CHILD_MAX];//子线程列表
// 	// p_proc->task.memmap.text_lin_base = 0;								 //initial这些段的数据并不清楚，在变身init的时候才在中赋新值
// 	// p_proc->task.memmap.text_lin_limit = 0;								 //initial这些段的数据并不清楚，在变身init的时候才在exec中赋新值
// 	// p_proc->task.memmap.data_lin_base = 0;								 //initial这些段的数据并不清楚，在变身init的时候才在exec中赋新值
// 	// p_proc->task.memmap.data_lin_limit = 0;								 //initial这些段的数据并不清楚，在变身init的时候才在exec中赋新值
// 	p_proc->task.memmap.vpage_lin_base = VpageLinBase;					 //保留内存基址
// 	p_proc->task.memmap.vpage_lin_limit = VpageLinBase;					 //保留内存界限

// 	return 0;

// }

void init_all_PCB()
{
	if(kernel_initial != 1)		return;
	// init_kernel_mm();

	PROCESS *p = proc_table;
	for(int i = 0; i < NR_PCBS; i++, p++){

		memset(p, 0, sizeof(PROCESS));

		p->task.stat = -1; 			//初始化状态 -1表示未初始化
		p->task.ticks = -1;
		p->task.priority = -1;
		p->task.pid = i;
		p->task.p_name[0] = (char)0;
		p->task.stat = FREE;
		// p->task.cwd[0] = (char)0;
		p->task.suspended = -1;
		p->task.exit_status = -1;
		p->task.child_exit_status = -1;
		p->task.sig_set = 0;
		p->task._Hanlder = NULL;
		p->task.retval = NULL;
		p->task.who_wait_flag = 0;

		// 开辟上下文帧的空间
		char *frame = (char *)(p + 1);
		frame -= sizeof(STACK_FRAME);
		p->task.context.esp_save_int = (STACK_FRAME*)frame;
		// 开辟中断帧的空间
		frame -= sizeof(CONTEXT_FRAME);
		p->task.context.esp_save_context = (CONTEXT_FRAME*)frame;

		init_cpu_context(&p->task.context, i, NULL, StackLinBase, (u32)restart_restore, PRIVILEGE_USER);

		for (int j = 0; j < NR_FILES; j++){
			proc_table[i].task.filp[j] = 0;
		}
		// 暂时不需要初始化mmu
	}
}

/*
 * @brief 初始化所有起始进程
 * @return 失败返回-1，成功返回initial进程的pid
 * @details 会初始化所有PCB，同时根据task_table创建task进程，task进程即服务进程；
 * 			创建initial进程，将p_proc_current和cr3_ready指向initial进程
 * 			创建idle进程
*/
PRIVATE int initialize_processes()
{

	init_all_PCB();
	for (int i = 0; i < NR_TASKS; i++){
		kthread_create(task_table[i].name, (void*)task_table[i].initial_eip, task_table[i].rt, PRIVILEGE_TASK);
	}

	int initial_pid = kthread_create("initial", initial, false, PRIVILEGE_TASK);

	/* When the first process begin running, a clock-interruption will happen immediately.
	 * If the first process's initial ticks is 1, it won't be the first process to execute its
	 * user code. Thus, it's will look weird, for proc_table[0] don't output first.
	 * added by xw, 18/4/19
	 */
	proc_table[0].task.ticks = 2;

	p_proc_current = &proc_table[initial_pid];
	cr3_ready = p_proc_current->task.cr3;

	return initial_pid;
}
