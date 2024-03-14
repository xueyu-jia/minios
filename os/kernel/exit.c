#include "type.h"
#include "const.h"
#include "proc.h"

/*======================================================================*
					  sys_exit			  added by mingxuan 2021-1-6
 *======================================================================*/
//PUBLIC void sys_exit(int status) //status为子进程返回的状态
//PUBLIC void do_exit(int status) //status为子进程返回的状态
/*
PUBLIC void kern_exit(int status) //status为子进程返回的状态
{

    //disp_str("\nexit\n");

    p_proc_current->task.we_flag = NORMAL;

    //exit_status
	p_proc_current->task.exit_status = status;

	//暂时不考虑过继, mingxuan 2021-1-6

	//暂时不考虑file exit, mingxuan 2021-1-6


	//如果有线程, 先把线程释放 mingxuan 2021-8-17
	u32 tid = -1;
	if(p_proc_current->task.info.child_t_num > 0) //说明有线程
	{
		//释放每个线程的栈物理页
		for(int i=0; i<p_proc_current->task.info.child_t_num; i++) //对每个子线程，都释放它的栈
		{
			tid = p_proc_current->task.info.child_thread[i]; //得到子线程的pid
			proc_table[tid].task.stat = IDLE; //释放线程的PCB, 防止在释放的过程中该线程被调度
			free_seg_phypage(tid, MEMMAP_STACK);
		}
		//释放每个线程的栈页表
		for(int i=0; i<p_proc_current->task.info.child_t_num; i++) //对每个子线程，都释放它的栈页表
		{
			tid = p_proc_current->task.info.child_thread[i]; //得到子线程的pid
			free_seg_pagetbl(tid, MEMMAP_STACK);
		}
	}
	//释放线程资源完毕，将自己的子线程数量设为0
	p_proc_current->task.info.child_p_num = 0;


	//释放进程的所有页地址空间
	free_all_phypage(p_proc_current->task.pid);
    free_all_pagetbl(p_proc_current->task.pid);
    //free_pagedir(p_proc_current->task.pid);	//不能释放cr3，不然会崩 //deleted by mingxuan 2021-1-7

	p_proc_current->task.info.child_p_num = 0;	//自己的子进程设为0，数量

	//判断是否有父进程
	if (p_proc_current->task.info.ppid==-1) //无父进程
	{
		p_proc_current->task.stat = IDLE;			//自己释放自己的进程表项
	}
	else //有父进程
	{
		p_proc_current->task.we_flag = ZOMBY;	//modified by hejia 2019-12-25
		p_proc_current->task.stat = SLEEPING;	//先置它为SLEEPING，因为当sched的时候产生进程调度，就不会调度它了（它的内存已经释放，如果再执行这个进程，肯定会发生错误）
												//但是不能设置为IDLE，因为可能会有其他的进程占据这个进程表项
	}

	//while(1);	//added by mingxuan 2021-8-17

	return;
}
*/

//modified by mingxuan 2021-8-21
PUBLIC void kern_exit(int status) //status为子进程返回的状态
{
    //disp_str("\nexit\n");
	PROCESS *p_proc = p_proc_current;	//p_proc表示exit应该处理的进程，该设计是为了兼容在线程中使用exit
	
	//检查是线程还是进程
	//added by mingxuan 2021-8-21
	if(TYPE_THREAD == p_proc->task.info.type)
	{
		//p_proc = &proc_table[p_proc->task.info.real_ppid];	//让exit直接处理它的亲父线程

		//由于存在线程创建线程的情况，所以需要一直向上层搜索，直到找到亲父进程（创建线程的进程）
		PROCESS *p_father = &proc_table[p_proc->task.info.real_ppid];
		while(TYPE_THREAD == p_father->task.info.type)
		{
			p_father = &proc_table[p_father->task.info.real_ppid];
		}
		p_proc = p_father;
	}

    //p_proc->task.we_flag = NORMAL;

    //exit_status
	p_proc->task.exit_status = status;

	//暂时不考虑过继, mingxuan 2021-1-6

	//过继机制，added by dongzhangqi 2023-4-14
	/*
	if(p_proc->task.info.child_p_num > 0){//有子进程
		for(int i = 0;i < NR_CHILD_MAX;i++){
			if(p_proc_current->task.info.child_process[i] !=0){//所有子进程都过继给init进程

				//子进程的父进程pid设为init进程的pid
				proc_table[p_proc_current->task.info.child_process[i]].task.info.ppid = NR_K_PCBS;

				//init进程的孩子数加一
				proc_table[NR_K_PCBS].task.info.child_p_num++;

				//更改init进程的子进程列表
				proc_table[NR_K_PCBS].task.info.child_process[i] = i; 

			}

		}

	}*/
	

	//暂时不考虑file exit, mingxuan 2021-1-6
	// 现在应该考虑了 xv6也测了这个 jiangfeng 2024-03-11
	vfs_put_dentry(p_proc->task.cwd);
	for(int i=0; i < NR_FILES; i++){
		if(p_proc->task.filp[i]){
			kern_vfs_close(i);
		}
	}
	//如果有线程, 先把线程释放 mingxuan 2021-8-17
	u32 tid = -1;
	if(p_proc->task.info.child_t_num > 0) //说明有线程
	{
		//防止在释放的过程中该线程被调度
		//added by mingxuan 2021-8-21
		for(int i=0; i<p_proc->task.info.child_t_num; i++)
		{
			tid = p_proc->task.info.child_thread[i];
			proc_table[tid].task.stat = FREE; //释放线程的PCB, 防止在释放的过程中该线程被调度 //统一PCB state 20240314
		}

		//释放每个线程的栈物理页
		for(int i=0; i<p_proc->task.info.child_t_num; i++) //对每个子线程，都释放它的栈
		{
			tid = p_proc->task.info.child_thread[i]; //得到子线程的pid
			free_seg_phypage(tid, MEMMAP_STACK);
		}

		//释放每个线程的栈页表
		for(int i=0; i<p_proc->task.info.child_t_num; i++) //对每个子线程，都释放它的栈页表
		{
			tid = p_proc->task.info.child_thread[i]; //得到子线程的pid
			free_seg_pagetbl(tid, MEMMAP_STACK);
		}

		//回收每个线程的PCB
		//added by mingxuan 2021-8-21
		for(int i=0; i<p_proc->task.info.child_t_num; i++) //对每个子线程，都释放它的栈
		{
			tid = p_proc->task.info.child_thread[i]; //得到子线程的pid
			free_PCB(&proc_table[tid]);
		}

	}
	//释放线程资源完毕，将自己的子线程数量设为0
	p_proc->task.info.child_t_num = 0;//modified by dongzhangqi 2023-4-12


	//释放进程的所有页地址空间
	free_all_phypage(p_proc->task.pid);
    free_all_pagetbl(p_proc->task.pid);
    //free_pagedir(p_proc_current->task.pid);	//不能释放cr3，不然会崩 //deleted by mingxuan 2021-1-7

	p_proc->task.info.child_p_num = 0;	//自己的子进程数量设为0

	//判断是否有父进程
	if (p_proc->task.info.ppid==-1) //无父进程
	{
		//p_proc->task.stat = IDLE;			//自己释放自己的进程表项
		free_PCB(p_proc);	//modified by mingxuan 2021-8-21
	}
	else //有父进程
	{
		//p_proc->task.we_flag = ZOMBY;	//modified by hejia 2019-12-25
		//p_proc->task.stat = SLEEPING;	//先置它为SLEEPING，因为当sched的时候产生进程调度，就不会调度它了（它的内存已经释放，如果再执行这个进程，肯定会发生错误）
												//但是不能设置为IDLE，因为可能会有其他的进程占据这个进程表项
		
		PROCESS *p_father = &proc_table[p_proc->task.info.ppid];
		p_proc->task.stat = KILLED; //modified by dongzhangqi 2023.6.2 //统一PCB state 20240314
		sys_wakeup(p_father);
		
	}

	//while(1);	//added by mingxuan 2021-8-17

	return;
}

PUBLIC void do_exit(int status)
{
	kern_exit(status);
}

// added by mingxuan 2021-8-13
PUBLIC void sys_exit()
{
    do_exit(get_arg(1));
}