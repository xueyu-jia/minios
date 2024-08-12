#include "const.h"
#include "proc.h"
#include "console.h"
#include "proto.h"
#include "spinlock.h"
#include "assert.h"

/*
在操作系统中，特别是在进程的状态管理中，"ZOMBIE"（僵尸）和"KILLED"（被杀死）是两种不同的状态，它们表示进程生命周期中的不同阶段：

ZOMBIE（僵尸状态）：

僵尸状态通常指的是子进程已经结束运行，但其父进程尚未通过调用如 wait 或 waitpid 这样的系统调用来检索其退出状态。
在这种状态下，子进程的进程控制块（PCB）依然存在，以便父进程能够获取退出码或其他退出相关信息。
僵尸进程不占用 CPU 时间，也不会消耗系统资源，但它们仍然占用一个进程ID（PID）和其他一些有限的资源。
父进程必须调用特定的系统调用来回收僵尸进程的状态信息，之后操作系统才会释放与该进程相关的资源。
KILLED（被杀死状态）：

被杀死状态指的是进程收到了一个信号（通常是 SIGKILL 或其他无法被忽略的致命信号），导致它被立即终止。
在这种状态下，进程可能需要进行一些清理工作，比如关闭打开的文件描述符、释放内存等，但这些操作通常是由操作系统或者回收站进程自动完成的。
与僵尸状态不同，被杀死的进程不会保留其 PCB 以供父进程检索状态，因为操作系统会认为该进程已经终止，且其状态不再重要。
被杀死的进程状态可能不会立即变为 "terminated"，直到操作系统的某个部分（如回收站进程）处理了该进程的资源回收。
try_get_zombie_child 函数用于查找已经结束但尚未被父进程回收的子进程，而 try_remove_killed_child 函数可能用于处理那些因为某些原因被终止的子进程。
总的来说，"ZOMBIE" 和 "KILLED" 状态的主要区别在于父进程是否有机会检索子进程的退出状态，以及操作系统如何处理这些进程的资源回收。
目前KILLED机制还未实现
*/

PRIVATE PROCESS* try_get_zombie_child(u32 pid);
PRIVATE int try_remove_killed_child(u32 pid);
PRIVATE void remove_zombie_child(u32 pid);

PUBLIC int kern_wait(int* wstatus)
{
    PROCESS* fa_pcb = p_proc_current;
    while (true) {
        lock_or_yield(&fa_pcb->task.lock);
        if (fa_pcb->task.tree_info.child_p_num == 0) {
            if (wstatus != NULL)	*wstatus = 0;
            release(&fa_pcb->task.lock);
            return -1;
        }

        PROCESS* exit_pcb = try_get_zombie_child(fa_pcb->task.pid);
        if (exit_pcb == NULL) {
            int pid = try_remove_killed_child(fa_pcb->task.pid);
            if (pid != -1) {
                release(&fa_pcb->task.lock);
                return pid;
            }
            // disable_int();
            // fa_pcb->task.stat = SLEEPING;
            release(&fa_pcb->task.lock);
			wait_event(fa_pcb);
            continue;
        }
        lock_or_yield(&exit_pcb->task.lock);
        remove_zombie_child(exit_pcb->task.pid);
		fa_pcb->task.child_exit_status = exit_pcb->task.exit_status;
		if (wstatus != NULL)	*wstatus = exit_pcb->task.exit_status;
        //! FIXME: no thread release here
        // wait_recycle_memory(exit_pcb->task.pid);
		// 内存资源已经在exit时就释放了

        int child_pid = exit_pcb->task.pid;

		disable_int();
		//释放子进程的进程表项，内存资源已经在exit时就释放了
		free_PCB(exit_pcb);	//modified by mingxuan 2021-8-21
        //! FIXME: lock also release here
        // assert(!exit_pcb->task.tree_info.child_k_num);
        // assert(!exit_pcb->task.tree_info.child_p_num);
        // assert(!exit_pcb->task.tree_info.child_t_num);
        // assert(!exit_pcb->task.tree_info.ppid);
        // assert(!exit_pcb->task.tree_info.real_ppid);
        release(&exit_pcb->task.lock);
        release(&fa_pcb->task.lock);
        enable_int();
        return child_pid;
    }
}

PRIVATE PROCESS* try_get_zombie_child(u32 pid)
{
    PROCESS* pcb = (PROCESS*)pid2proc(pid);
    for (int i = 0; i < pcb->task.tree_info.child_p_num; i++) {
        PROCESS* exit_child = (PROCESS*)pid2proc(pcb->task.tree_info.child_process[i]);
        if (exit_child->task.stat == ZOMBY) return exit_child;
    }
    return NULL;
}

PRIVATE int try_remove_killed_child(u32 pid)
{
	// todo：完善KILLED状态机制
	return -1;
}
// static int try_remove_killed_child(u32 pid) {
//     PROCESS* pcb = (PROCESS*)pid2proc(pid);
//     if (pcb->task.tree_info.child_k_num == 0) { return -1; }
//     for (int i = pcb->task.tree_info.child_k_num - 1; i >= 0; --i) {
//         --pcb->task.tree_info.child_k_num;
//         return (int)pcb->task.tree_info.child_killed[i];
//     }
//     panic("unreachable");
// }

/**
 * @brief	从child_process数组中移除zomby进程，并更新child_process数组
 */
PRIVATE void remove_zombie_child(u32 pid)
{
    PROCESS* pcb = p_proc_current;
    bool   cpyflg = false;
    for (int i = 0; i < pcb->task.tree_info.child_p_num; i++) {
        PROCESS* exit_child = (PROCESS*)pid2proc(pcb->task.tree_info.child_process[i]);
        assert(!(cpyflg && exit_child->task.pid == pid));
        if (exit_child->task.pid == pid)	cpyflg = true;
        if (cpyflg && i < pcb->task.tree_info.child_p_num - 1) {
            pcb->task.tree_info.child_process[i] =
                pcb->task.tree_info.child_process[i + 1];
        }
    }
    assert(cpyflg);
    pcb->task.tree_info.child_p_num--;
    return;
}


// /*======================================================================*
//                       sys_wait            added by mingxuan 2021-1-6
//  *======================================================================*/
// //PUBLIC int sys_wait(void)
// PUBLIC int kern_wait(int *status) //wait返回的为子进程pid,子进程退出状态通过*status传递 modified by dongzhangqi 2023-4-20
// {
// 	int child_pid = -1;
//     u32 exit_child_hanging = 0;
// 	PROCESS* p_proc = NULL;
// 	//看有无子进程，如果没有子进程就打印错误，并返回-1
// 	if(p_proc_current->task.tree_info.child_p_num == 0)
// 	{
// 		disp_str("no child_process!! error\n");
// 		return -1;
// 	}

//     //遍历子进程看有无正ZOMBY的
// 	while(!exit_child_hanging)
// 	{
//         int i;
// 		lock_or_yield(&p_proc_current->task.lock);
// 		for(i = 0;i < NR_CHILD_MAX;i++)
// 		{
// 			//该子进程表项中有值且处于ZOMBY
// 			if(p_proc_current->task.tree_info.child_process[i] !=0 && proc_table[p_proc_current->task.tree_info.child_process[i]].task.stat == ZOMBY)
// 			{
// 				exit_child_hanging = 1;

// 				//置子进程的状态为正常
// 				//proc_table[p_proc_current->task.tree_info.child_process[i]].task.we_flag = NORMAL;
// 				//proc_table[p_proc_current->task.tree_info.child_process[i]].task.stat = ZOMBY;

// 				//取走子进程的exit_status，并赋值给自己的child_exit_status
// 				p_proc = &proc_table[p_proc_current->task.tree_info.child_process[i]];
// 				p_proc_current->task.child_exit_status = p_proc->task.exit_status;

// 				//父进程的孩子数减一
// 				p_proc_current->task.tree_info.child_p_num--;

// 				//更新父进程的孩子数组
// 				p_proc_current->task.tree_info.child_process[i] = 0;
// 				/*added by dongzhangqi 2023.4.20
// 				修改wait的返回值，参考Linux的设计int wait(int *status)
// 				子进程的pid作为wait()的返回值，子进程的退出状态exit_status通过 *status来传递
// 				*/
// 				//取走子进程的pid，用作wait的返回值
// 				child_pid = p_proc->task.pid;

// 				//exit status通过*status传递
// 				if(status != NULL){
// 					*status = p_proc_current->task.child_exit_status;
// 				}

// 				//释放子进程的进程表项
// 				free_PCB(p_proc);	//modified by mingxuan 2021-8-21

// 				//将自己的状态置为正常
// 				//p_proc_current->task.we_flag = NORMAL;


// 				//只释放一个
// 				break;
// 			}
// 		}
// 		//如果没有子进程在ZOMBY
// 		if(exit_child_hanging == 0)
// 		{
// 			//p_proc_current->task.we_flag = WAITING;
// 			release(&p_proc_current->task.lock);
// 			wait_event(p_proc_current); //changed by dongzhangqi 2023.6.2
// 		}
// 	}
// 	release(&p_proc_current->task.lock);
//     //return p_proc_current->task.child_exit_status;;
// 	return child_pid;
// }

PUBLIC int do_wait(int *status) //wait返回的为子进程pid,子进程退出状态通过*status传递 modified by dongzhangqi 2023-4-20
{
	return kern_wait(status);
}

PUBLIC int sys_wait() //wait返回的为子进程pid modified by dongzhangqi 2023-4-20
{
	return do_wait((int*)get_arg(1));
}
