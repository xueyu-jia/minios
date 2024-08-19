#include <kernel/console.h>
#include <klib/spinlock.h>
#include <kernel/type.h>
#include <kernel/const.h>
#include <kernel/proc.h>
#include <kernel/pagetable.h>
#include <kernel/memman.h>
#include <kernel/proto.h>
#include <klib/assert.h>
// #include <kernel/exit.h>

PRIVATE void exit_file(PROCESS* p_proc);
PRIVATE int transfer_child_proc(u32 src_pid, u32 dst_pid);
PRIVATE void exit_handle_child_thread(u32 pid, bool lock_recy);

/**
 * @brief	结束进程
 * @note	结束当前进程，释放进程所有资源，将子进程过继给回收进程，回收进程的pid见宏定义NR_RECY_PROC
 */
PUBLIC void kern_exit(int exit_code)
{
    PROCESS *exit_pcb = proc_real(p_proc_current); //线程执行退出，则退出整个进程
    PROCESS *fa_pcb   = pid2proc(exit_pcb->task.tree_info.ppid);
    PROCESS *recy_pcb = (PROCESS*)pid2proc(NR_RECY_PROC);
    // 释放所有文件资源 xv6也测了这个 jiangfeng 2024-03-11
	// 可能触发文件同步和硬盘IO,在上锁之前完成
	exit_file(exit_pcb);
	lock_or_yield(&exit_pcb->task.lock);

	lock_or_yield(&fa_pcb->task.lock);
    assert(exit_pcb->task.tree_info.ppid >= 0 && exit_pcb->task.tree_info.ppid < NR_PCBS);
    assert(fa_pcb->task.stat == READY || fa_pcb->task.stat == SLEEPING);

	// 处理已退出的子进程 todo 需要修改数据结构TREE_INFO，增加KILLED机制
    // exit_handle_child_killed_proc(exit_pcb->task.pid);

	//释放进程的所有页地址空间
	memmap_clear(exit_pcb);
    free_all_pagetbl(exit_pcb->task.pid);
    free_pagedir(exit_pcb->task.pid);

    //: NOTE: disable int to reduce op complexity
    if (fa_pcb->task.pid == NR_RECY_PROC) {
        //: case 0: father is recy and already locked
        exit_handle_child_thread(exit_pcb->task.pid, false);
        transfer_child_proc(exit_pcb->task.pid, NR_RECY_PROC);

    } else {
        //: case 1: father isn't recy so need to lock
        exit_handle_child_thread(exit_pcb->task.pid, true);
        lock_or_yield(&recy_pcb->task.lock);
        if (transfer_child_proc(exit_pcb->task.pid, NR_RECY_PROC) != 0) {
            disable_int();
            recy_pcb->task.stat = READY;
            enable_int();
        }
        release(&recy_pcb->task.lock);
    }
    disable_int();
    exit_pcb->task.stat      	= ZOMBY;
    exit_pcb->task.exit_status 	= exit_code;
	out_rq(exit_pcb);
    enable_int();
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
    //: NOTE:fixed, now recursive delete
    PROCESS* pcb      = (PROCESS*)pid2proc(pid);
    PROCESS* recy_pcb = (PROCESS*)pid2proc(NR_RECY_PROC);
    for (int i = 0; i < pcb->task.tree_info.child_t_num; ++i) {
        PROCESS* child_pcb = (PROCESS*)pid2proc(pcb->task.tree_info.child_thread[i]);
		// mmu 重构后，子线程直接引用父进程的memmap, 故不用单独释放子线程的内存空间
        // 子线程禁止拥有进程和线程
        // for (int i = 0; i < child_pcb->task.tree_info.child_t_num; ++i) {
        //     exit_handle_child_thread(child_pcb->task.tree_info.child_thread[i], lock_recy);
        // }
		// // 处理子线程拥有的子进程
        // if (lock_recy) {
        //     lock_or_yield(&recy_pcb->task.lock);
        //     transfer_child_proc(child_pcb->task.pid, NR_RECY_PROC);
        //     release(&recy_pcb->task.lock);
        // } else {
        //     transfer_child_proc(child_pcb->task.pid, NR_RECY_PROC);
        // }
		// free PCBs of child thread
        child_pcb->task.stat = ZOMBY;
		free_PCB(child_pcb);
    }
    pcb->task.tree_info.child_t_num = 0;
    return;
}
