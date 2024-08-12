#include "console.h"
#include "spinlock.h"
#include "type.h"
#include "const.h"
#include "proc.h"
#include "pagetable.h"
#include "memman.h"
#include "proto.h"
#include "assert.h"

PRIVATE void exit_file(PROCESS* p_proc);
PRIVATE int transfer_child_proc(u32 src_pid, u32 dst_pid);
PRIVATE void exit_handle_child_thread(u32 pid, bool lock_recy);

/**
 * @brief	结束进程
 * @note	结束当前进程，释放进程所有资源，将子进程过继给回收进程，回收进程的pid见宏定义NR_RECY_PROC
 */
PUBLIC void kern_exit(int exit_code)
{
    PROCESS *exit_pcb = NULL;
    PROCESS *fa_pcb   = NULL;
    PROCESS *recy_pcb = (PROCESS*)pid2proc(NR_RECY_PROC);

	exit_pcb = (PROCESS*)pid2proc(p_proc_current->task.pid);
	lock_or_yield(&exit_pcb->task.lock);
	fa_pcb = (PROCESS*)pid2proc(exit_pcb->task.tree_info.ppid);

	// 释放所有文件资源 xv6也测了这个 jiangfeng 2024-03-11
	// 可能触发文件同步和硬盘IO,在上锁之前完成
	exit_file(exit_pcb);

	lock_or_yield(&fa_pcb->task.lock);
    assert(exit_pcb->task.tree_info.ppid >= 0 && exit_pcb->task.tree_info.ppid < NR_PCBS);
    assert(fa_pcb->task.stat == READY || fa_pcb->task.stat == SLEEPING);

	// 处理已退出的子进程 todo 需要修改数据结构TREE_INFO
    // exit_handle_child_killed_proc(exit_pcb->task.pid);

	//释放进程的所有页地址空间
	// free_all_phypage(p_proc->task.pid);
	memmap_clear(exit_pcb);
    free_all_pagetbl(exit_pcb->task.pid);
    free_pagedir(exit_pcb->task.pid);

    //! NOTE: disable int to reduce op complexity
    if (fa_pcb->task.pid == NR_RECY_PROC) {
        //! case 0: father is recy and already locked
        exit_handle_child_thread(exit_pcb->task.pid, false);
        transfer_child_proc(exit_pcb->task.pid, NR_RECY_PROC);

    } else {
        //! case 1: father isn't recy so need to lock
        exit_handle_child_thread(exit_pcb->task.pid, true);
        lock_or_yield(&recy_pcb->task.lock);
        if (transfer_child_proc(exit_pcb->task.pid, NR_RECY_PROC) != 0) {
            recy_pcb->task.stat = READY;
        }
        release(&recy_pcb->task.lock);
    }

    exit_pcb->task.stat      	= KILLED;
    exit_pcb->task.exit_status 	= exit_code;
	out_rq(exit_pcb);

    // assert(exit_pcb->task.lock);
    // assert(fa_pcb->task.lock);

    release(&exit_pcb->task.lock);
    release(&fa_pcb->task.lock);
	wakeup(fa_pcb);
    sched_yield();
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

// // 将p_proc的所有子进程转移给p_proc_init
// // req. p_proc_init->task.lock p_proc->task.lock **注意上锁顺序，防死锁**
// PRIVATE void transfer_child_process(PROCESS* p_proc, PROCESS* p_proc_init) {
// 	if(p_proc->task.tree_info.child_p_num > 0){//有子进程
// 	// 函数拆分 jiangfeng 2024-05-05
// 	// pcb lock 注意上锁顺序 jiangfeng 2024-05-05
// 		lock_or_yield(&p_proc_init->task.lock);
// 		lock_or_yield(&p_proc->task.lock);
// 		for(int i = 0;i < NR_CHILD_MAX;i++){
// 			if(p_proc_current->task.tree_info.child_process[i] !=0){//所有子进程都过继给init进程
// 				PROCESS* p_child = &proc_table[p_proc_current->task.tree_info.child_process[i]];
// 				lock_or_yield(&p_child->task.lock);
// 				//子进程的父进程pid设为init进程的pid
// 				p_child->task.tree_info.ppid = PID_INIT;

// 				//init进程的孩子数加一
// 				int index = p_proc_init->task.tree_info.child_p_num++;

// 				//更改init进程的子进程列表
// 				p_proc_init->task.tree_info.child_process[index] = proc2pid(p_child);
// 				release(&p_child->task.lock);
// 			}

// 		}
// 		p_proc->task.tree_info.child_p_num = 0; //自己的子进程数量设为0
// 		release(&p_proc->task.lock);
// 		release(&p_proc_init->task.lock);
// 	}
// }
/**
 * @brief	将进程src_pid 的子进程过继给进程dst_pid
 * @note	调用该函数前请持有dst_pid的锁
 * @retval	返回src_pid的子进程数目
 */
PRIVATE int transfer_child_proc(u32 src_pid, u32 dst_pid)
{
    int number = 0, index = 0;
    assert(src_pid != dst_pid);
    PROCESS* src_pcb = (PROCESS*)pid2proc(src_pid);
    PROCESS* dst_pcb = (PROCESS*)pid2proc(dst_pid);
    for (int i = 0; i < src_pcb->task.tree_info.child_p_num; i++) {
		index = dst_pcb->task.tree_info.child_p_num++;
		// todo:错误处理
		// assert(index < NR_CHILD_MAX);
		if(index >= NR_CHILD_MAX) {
			dst_pcb->task.tree_info.child_p_num--;
			break;
		}
        dst_pcb->task.tree_info.child_process[index] = src_pcb->task.tree_info.child_process[i];
        PROCESS* son_pcb = (PROCESS*)pid2proc(src_pcb->task.tree_info.child_process[i]);

        lock_or_yield(&son_pcb->task.lock);
        son_pcb->task.tree_info.ppid = dst_pid;
        dst_pcb->task.tree_info.child_p_num++;
        release(&son_pcb->task.lock);
    }
    number = src_pcb->task.tree_info.child_p_num;
    src_pcb->task.tree_info.child_p_num = 0;
    return number;
}

// 释放file的引用计数
PRIVATE void exit_file(PROCESS* p_proc)
{
	vfs_put_dentry(p_proc->task.cwd);
	for(int i=0; i < NR_FILES; i++){
		if(p_proc->task.filp[i]){
			kern_vfs_close(i);
		}
	}
}

// /**
//  * @brief 用于exit清理退出进程拥有的线程
//  */
// PRIVATE void exit_thread(PROCESS* p_proc) {
// 	u32 tid = -1;
// 	if(p_proc->task.tree_info.child_t_num > 0) //说明有线程
// 	{
// 		//防止在释放的过程中该线程被调度
// 		//added by mingxuan 2021-8-21
// 		for(int i=0; i<p_proc->task.tree_info.child_t_num; i++)
// 		{
// 			tid = p_proc->task.tree_info.child_thread[i];
// 			out_rq(pid2proc(tid));
// 			proc_table[tid].task.stat = KILLED; //释放线程的PCB, 防止在释放的过程中该线程被调度 //统一PCB state 20240314
// 		}


// 		// mmu 重构后，子线程直接引用父进程的memmap, 故不用单独释放

// 		//回收每个线程的PCB
// 		//added by mingxuan 2021-8-21
// 		for(int i=0; i<p_proc->task.tree_info.child_t_num; i++) //对每个子线程，都释放它的栈
// 		{
// 			tid = p_proc->task.tree_info.child_thread[i]; //得到子线程的pid
// 			free_PCB(&proc_table[tid]);
// 		}

// 	}
// 	//释放线程资源完毕，将自己的子线程数量设为0
// 	p_proc->task.tree_info.child_t_num = 0;//modified by dongzhangqi 2023-4-12
// }

/**
 * @brief 	用于exit清理退出进程拥有的线程
 * @param	lock_recy:进程的父进程是否是回收进程
 * @note	遍历进程的所有子线程
				1、mmu 重构后，子线程直接引用父进程的memmap, 故不用单独释放子线程的内存空间
				2、递归处理子线程的子线程
				3、处理子线程拥有的的子进程
				4、释放子线程占有的PCB
 */

PRIVATE void exit_handle_child_thread(u32 pid, bool lock_recy)
{
    //! NOTE:fixed, now recursive delete
    PROCESS* pcb      = (PROCESS*)pid2proc(pid);
    PROCESS* recy_pcb = (PROCESS*)pid2proc(NR_RECY_PROC);
    for (int i = 0; i < pcb->task.tree_info.child_t_num; ++i) {
        PROCESS* child_pcb = (PROCESS*)pid2proc(pcb->task.tree_info.child_thread[i]);
        // u32    cr3       = ((PROCESS*)pid2proc(pid))->task.cr3;
        // u32    laddr     = child_pcb->task.memmap.stack_lin_limit;
        // u32    limit     = child_pcb->task.memmap.stack_lin_base;
        // while (laddr < limit) {
        //     bool ok = pg_unmap_laddr(cr3, laddr, true);
        //     assert(ok);
        //     laddr = pg_frame_phyaddr(laddr) + NUM_4K;
        // }
		// mmu 重构后，子线程直接引用父进程的memmap, 故不用单独释放子线程的内存空间
        for (int i = 0; i < child_pcb->task.tree_info.child_t_num; ++i) {
            exit_handle_child_thread(child_pcb->task.tree_info.child_thread[i], lock_recy);
        }
		// 处理子线程拥有的子进程
        if (lock_recy) {
            lock_or_yield(&recy_pcb->task.lock);
            transfer_child_proc(child_pcb->task.pid, NR_RECY_PROC);
            release(&recy_pcb->task.lock);
        } else {
            transfer_child_proc(child_pcb->task.pid, NR_RECY_PROC);
        }
		// free PCBs of child thread
        child_pcb->task.stat = KILLED;
		free_PCB(child_pcb);		// todo：不太明白KILLED状态存在的意义
    }
    pcb->task.tree_info.child_t_num = 0;
    return;
}


/*======================================================================*
					  kern_exit			  added by mingxuan 2021-1-6
 *======================================================================*/

// // modified jiangfeng 2024-05-05 函数功能拆分
// // 退出当前进程，杀死所有线程并释放内存和文件资源，如果有父进程则唤醒之
// // @param status: 当前进程的退出状态
// PUBLIC void kern_exit(int status) //status为子进程返回的状态
// {

// 	PROCESS *p_proc = proc_real(p_proc_current); //线程执行退出，则退出整个进程
// 	PROCESS *p_proc_init = &proc_table[PID_INIT];
//     //p_proc->task.we_flag = NORMAL;

// 	//过继机制，added by dongzhangqi 2023-4-14
// 	transfer_child_process(p_proc, p_proc_init);

// 	// 释放所有文件资源 xv6也测了这个 jiangfeng 2024-03-11
// 	// 可能触发文件同步和硬盘IO,在上锁之前完成
// 	exit_file(p_proc);
// 	lock_or_yield(&p_proc->task.lock);
//     //exit_status
// 	p_proc->task.exit_status = status;


// 	//如果有线程, 先把线程释放 mingxuan 2021-8-17
// 	exit_thread(p_proc);

// 	//释放进程的所有页地址空间
// 	// free_all_phypage(p_proc->task.pid);
// 	memmap_clear(p_proc);
//     free_all_pagetbl(p_proc->task.pid);
//     free_pagedir(p_proc->task.pid);

// 	//判断是否有父进程
// 	if (p_proc->task.tree_info.ppid==-1) //无父进程
// 	{
// 		//p_proc->task.stat = IDLE;			//自己释放自己的进程表项
// 		release(&p_proc->task.lock);
// 		release(&p_proc_init->task.lock);
// 		free_PCB(p_proc);	//modified by mingxuan 2021-8-21
// 	}
// 	else //有父进程
// 	{
// 		//p_proc->task.we_flag = ZOMBY;	//modified by hejia 2019-12-25
// 		//p_proc->task.stat = SLEEPING;	//先置它为SLEEPING，因为当sched的时候产生进程调度，就不会调度它了（它的内存已经释放，如果再执行这个进程，肯定会发生错误）
// 												//但是不能设置为IDLE，因为可能会有其他的进程占据这个进程表项
// 		disable_int();
// 		PROCESS *p_father = &proc_table[p_proc->task.tree_info.ppid];
// 		p_proc->task.stat = KILLED; //modified by dongzhangqi 2023.6.2 //统一PCB state 20240314
// 		out_rq(p_proc);
// 		release(&p_proc->task.lock);
// 		release(&p_proc_init->task.lock);
// 		wakeup(p_father);
// 		enable_int();
// 	}
// 	sched_yield();

// 	//while(1);	//added by mingxuan 2021-8-17

// 	return;
// }
