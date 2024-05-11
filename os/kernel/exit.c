#include "type.h"
#include "const.h"
#include "proc.h"
#include "pagetable.h"
#include "memman.h"

// 将p_proc的所有子进程转移给p_proc_init
// req. p_proc_init->task.lock p_proc->task.lock **注意上锁顺序，防死锁**
PRIVATE transfer_child_process(PROCESS* p_proc, PROCESS* p_proc_init) {
	if(p_proc->task.info.child_p_num > 0){//有子进程
		for(int i = 0;i < NR_CHILD_MAX;i++){
			if(p_proc_current->task.info.child_process[i] !=0){//所有子进程都过继给init进程
				PROCESS* p_child = &proc_table[p_proc_current->task.info.child_process[i]];
				//子进程的父进程pid设为init进程的pid
				p_child->task.info.ppid = PID_INIT;

				//init进程的孩子数加一
				int index = p_proc_init->task.info.child_p_num++;

				//更改init进程的子进程列表
				p_proc_init->task.info.child_process[index] = proc2pid(p_child); 

			}

		}
		p_proc->task.info.child_p_num = 0; //自己的子进程数量设为0
	}
}

// 释放file的引用计数
PRIVATE void exit_file(PROCESS* p_proc) {
	vfs_put_dentry(p_proc->task.cwd);
	for(int i=0; i < NR_FILES; i++){
		if(p_proc->task.filp[i]){
			kern_vfs_close(i);
		}
	}
}

PRIVATE void exit_thread(PROCESS* p_proc) {
	u32 tid = -1;
	if(p_proc->task.info.child_t_num > 0) //说明有线程
	{
		//防止在释放的过程中该线程被调度
		//added by mingxuan 2021-8-21
		for(int i=0; i<p_proc->task.info.child_t_num; i++)
		{
			tid = p_proc->task.info.child_thread[i];
			out_rq(pid2proc(tid));
			proc_table[tid].task.stat = KILLED; //释放线程的PCB, 防止在释放的过程中该线程被调度 //统一PCB state 20240314
		}

		// //释放每个线程的栈物理页
		// for(int i=0; i<p_proc->task.info.child_t_num; i++) //对每个子线程，都释放它的栈
		// {
		// 	tid = p_proc->task.info.child_thread[i]; //得到子线程的pid
		// 	free_seg_phypage(tid, MEMMAP_STACK);
		// }

		// //释放每个线程的栈页表
		// for(int i=0; i<p_proc->task.info.child_t_num; i++) //对每个子线程，都释放它的栈页表
		// {
		// 	tid = p_proc->task.info.child_thread[i]; //得到子线程的pid
		// 	free_seg_pagetbl(tid, MEMMAP_STACK);
		// }
		// mmu 重构后，子线程直接引用父进程的memmap, 故不用单独释放

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
}
/*======================================================================*
					  kern_exit			  added by mingxuan 2021-1-6
 *======================================================================*/

// modified jiangfeng 2024-05-05 函数功能拆分
// 退出当前进程，杀死所有线程并释放内存和文件资源，如果有父进程则唤醒之
// @param status: 当前进程的退出状态
PUBLIC void kern_exit(int status) //status为子进程返回的状态
{

	PROCESS *p_proc = proc_real(p_proc_current); //线程执行退出，则退出整个进程
	PROCESS *p_proc_init = &proc_table[PID_INIT];
    //p_proc->task.we_flag = NORMAL;

	// pcb lock 注意上锁顺序 jiangfeng 2024-05-05
	lock_or_yield(&p_proc_init->task.lock);
	lock_or_yield(&p_proc->task.lock);
    //exit_status
	p_proc->task.exit_status = status;

	//过继机制，added by dongzhangqi 2023-4-14
	// 函数拆分 jiangfeng 2024-05-05
	transfer_child_process(p_proc, p_proc_init);

	//如果有线程, 先把线程释放 mingxuan 2021-8-17
	exit_thread(p_proc);
	// 释放所有文件资源 xv6也测了这个 jiangfeng 2024-03-11
	release(&p_proc->task.lock); // 可能触发硬盘写回临时解除锁
	exit_file(p_proc);
	lock_or_yield(&p_proc->task.lock);

	//释放进程的所有页地址空间
	// free_all_phypage(p_proc->task.pid);
	memmap_clear(p_proc);
    free_all_pagetbl(p_proc->task.pid);
    free_pagedir(p_proc->task.pid);

	//判断是否有父进程
	if (p_proc->task.info.ppid==-1) //无父进程
	{
		//p_proc->task.stat = IDLE;			//自己释放自己的进程表项
		release(&p_proc->task.lock);
		release(&p_proc_init->task.lock);
		free_PCB(p_proc);	//modified by mingxuan 2021-8-21
	}
	else //有父进程
	{
		//p_proc->task.we_flag = ZOMBY;	//modified by hejia 2019-12-25
		//p_proc->task.stat = SLEEPING;	//先置它为SLEEPING，因为当sched的时候产生进程调度，就不会调度它了（它的内存已经释放，如果再执行这个进程，肯定会发生错误）
												//但是不能设置为IDLE，因为可能会有其他的进程占据这个进程表项
		disable_int();
		PROCESS *p_father = &proc_table[p_proc->task.info.ppid];
		p_proc->task.stat = KILLED; //modified by dongzhangqi 2023.6.2 //统一PCB state 20240314
		out_rq(p_proc);
		release(&p_proc->task.lock);
		release(&p_proc_init->task.lock);
		wakeup(p_father);
		enable_int();
	}
	sched_yield();

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