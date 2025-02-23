#include <minios/wait.h>
#include <minios/proc.h>
#include <minios/sched.h>
#include <minios/console.h>
#include <minios/assert.h>
#include <minios/spinlock.h>
#include <minios/interrupt.h>
#include <compiler.h>

/*
在Linux操作系统中，"ZOMBIE"（僵尸）和"KILLED"（被杀死）这两个术语描述了进程的不同终止状态，但它们并不是官方的进程状态。
下面是它们的区别：

ZOMBIE状态：
僵尸状态是一种特殊的进程状态，发生在子进程终止后，但父进程尚未读取其退出状态时。
僵尸进程保留了足够的信息以供父进程查询其退出状态，但不再消耗系统资源或CPU时间。
僵尸进程的进程描述符（PCB）仍然存在于系统中，直到父进程调用wait()或waitpid()系统调用来回收其信息。
僵尸进程在ps命令的输出中通常用"Z"表示。

KILLED状态：
"KILLED"不是一个官方的Linux进程状态，而是通常用来描述一个已经被SIGKILL信号终止的进程。
当进程接收到SIGKILL信号时，它会被操作系统立即终止，没有机会执行任何清理或关闭操作。
与僵尸状态不同，被SIGKILL的进程不会保留其进程描述符或状态信息，因为它是立即且彻底地终止的。
由于进程立即终止，它不会进入等待或僵尸状态，也不会在ps命令的输出中显示为"KILLED"。实际上，它将从进程列表中消失。
资源回收：

在僵尸状态下，系统保留了进程的一些信息，但不会占用CPU资源或其他系统资源。
对于被SIGKILL的进程，系统会立即回收其所有资源，包括内存、文件描述符等。
父进程通知：

对于僵尸进程，父进程可以通过特定的系统调用来获取子进程的退出状态。
对于被SIGKILL的进程，父进程可能会收到一个SIGCHLD信号，但无法获取子进程的退出状态，因为子进程没有机会进入僵尸状态。
总结来说，"ZOMBIE"状态是子进程已经终止但尚未被父进程回收的状态，而"KILLED"状态描述的是进程已经被SIGKILL信号立即终止的情况。
在Linux中，进程的官方状态有运行（R）、睡眠（S）、停止（T）和僵尸（Z）等，但没有"KILLED"这个状态。
目前KILLED机制还未实现
*/

static process_t* try_get_zombie_child(u32 pid);
static int try_remove_killed_child(u32 pid);
static void remove_zombie_child(u32 pid);

int kern_wait(int* wstatus) {
    process_t* fa_pcb = p_proc_current;
    while (true) {
        spinlock_lock_or_yield(&fa_pcb->task.lock);
        if (fa_pcb->task.tree_info.child_p_num == 0) {
            if (wstatus != NULL) *wstatus = 0;
            spinlock_release(&fa_pcb->task.lock);
            kprintf("error: wait: found no child\n");
            return -1;
        }

        process_t* exit_pcb = try_get_zombie_child(fa_pcb->task.pid);
        if (exit_pcb == NULL) {
            int pid = try_remove_killed_child(fa_pcb->task.pid);
            if (pid != -1) {
                spinlock_release(&fa_pcb->task.lock);
                return pid;
            }
            disable_int_begin();
            spinlock_release(&fa_pcb->task.lock);
            wait_event(fa_pcb);
            //! FIXME: we sleep before turn on (maybe) the ints, it does work well but is it surely
            //! right? is the nearest turn on action comes from the idle proc?
            disable_int_end();
            continue;
        }

        remove_zombie_child(exit_pcb->task.pid);
        //! FIXME: no thread spinlock_release here
        // wait_recycle_memory(exit_pcb->task.pid);
        // 内存资源已经在exit时就释放了

        fa_pcb->task.child_exit_status = exit_pcb->task.exit_status;
        int child_pid = exit_pcb->task.pid;
        spinlock_lock_or_yield(&exit_pcb->task.lock);
        // 释放子进程的进程表项
        free_pcb(exit_pcb); // modified by mingxuan 2021-8-21
        spinlock_release(&exit_pcb->task.lock);
        spinlock_release(&fa_pcb->task.lock);
        if (wstatus != NULL) *wstatus = fa_pcb->task.child_exit_status;
        return child_pid;
    }
}

static process_t* try_get_zombie_child(u32 pid) {
    process_t* pcb = (process_t*)pid2proc(pid);
    for (int i = 0; i < pcb->task.tree_info.child_p_num; ++i) {
        process_t* exit_child = (process_t*)pid2proc(pcb->task.tree_info.child_process[i]);
        if (exit_child->task.stat == ZOMBY) return exit_child;
    }
    return NULL;
}

static int try_remove_killed_child(u32 pid) {
    UNUSED(pid);
    //! TODO: 完善KILLED状态机制
    return -1;
}

/**
 * @brief	从child_process数组中移除zomby进程，并更新child_process数组
 */
static void remove_zombie_child(u32 pid) {
    process_t* pcb = p_proc_current;
    bool cpyflg = false;
    for (int i = 0; i < pcb->task.tree_info.child_p_num; ++i) {
        process_t* exit_child = (process_t*)pid2proc(pcb->task.tree_info.child_process[i]);
        assert(!(cpyflg && exit_child->task.pid == pid));
        if (exit_child->task.pid == pid) cpyflg = true;
        if (cpyflg && i < pcb->task.tree_info.child_p_num - 1) {
            pcb->task.tree_info.child_process[i] = pcb->task.tree_info.child_process[i + 1];
        }
    }
    assert(cpyflg);
    pcb->task.tree_info.child_p_num--;
}
