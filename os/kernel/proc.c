
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <kernel/buffer.h>
#include <kernel/clock.h>
#include <kernel/console.h>
#include <kernel/const.h>
#include <kernel/hd.h>
#include <kernel/mmap.h>
#include <kernel/pagetable.h>
#include <kernel/proc.h>
#include <kernel/proto.h>
#include <kernel/type.h>
#include <kernel/x86.h>
#include <klib/string.h>

u32 cr3_ready;
int u_proc_sum;
int kernel_initial;
PROCESS* p_proc_current;
PROCESS* p_proc_next;
PUBLIC PROCESS cpu_table[NR_CPUS];
PUBLIC PROCESS proc_table[NR_PCBS];
PUBLIC TASK task_table[NR_TASKS] = {
    {idle, 0, RPL_KRNL, 0, PROC_NICE_MAX, STACK_SIZE_TASK, "task_idle"},
    {task_tty, 1, RPL_TASK, 1, 1, STACK_SIZE_TASK, "task_tty"},
    {hd_service, 1, RPL_TASK, 1, 2, STACK_SIZE_TASK, "hd_service"},
    // {bsync_service, 1, RPL_TASK, 1, 3,             STACK_SIZE_TASK, "bsync" }
};

PRIVATE int ldt_seg_linear(PROCESS* p, int idx);

PUBLIC int kern_get_pid() {
    return p_proc_current->task.pid;
}

// PUBLIC int do_get_pid()
// {
//     return kern_get_pid();
// }

// PUBLIC int sys_get_pid()
// {
//     return do_get_pid();
// }

PUBLIC int kern_get_pid_byname(char* name) {
    for (PROCESS* proc = proc_table; proc < proc_table + NR_PCBS; proc++) {
        if (strcmp(proc->task.p_name, name) == 0) { return proc->task.pid; }
    }
    return -1;
}

// PUBLIC int do_get_pid_byname(char* name)
// {
//     return kern_get_pid_byname(name);
// }

// PUBLIC int sys_get_pid_byname()
// {
//     return do_get_pid_byname((char*)get_arg(1));
// }

PUBLIC void kern_get_proc_msg(proc_msg* msg) {
    msg->pid = p_proc_current->task.pid;
    msg->nice = p_proc_current->task.nice;
    msg->sum_cpu_use = p_proc_current->task.sum_cpu_use;
    msg->vruntime = p_proc_current->task.vruntime;
}

PUBLIC void do_get_proc_msg(proc_msg* msg) {
    kern_get_proc_msg(msg);
}
PUBLIC void sys_get_proc_msg() {
    do_get_proc_msg((proc_msg*)get_arg(1));
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void wait_for_sem(void* chan, struct spinlock* lk) {
    if (0 == p_proc_current)
        // printf("p_proc_current is unknow!");
        return;
    if (0 == lk)
        // printf("lk is unknow!");
        return;
    // Must acquire ptable.lock in order to
    // change p->state and then call sched.
    // Once we hold ptable.lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup runs with ptable.lock locked),
    // so it's okay to release lk.

    // Go to sleep.
    disable_int();
    p_proc_current->task.channel = chan;
    p_proc_current->task.stat = SLEEPING;
    rq_remove(p_proc_current);
    release(lk);
    enable_int();

    sched_yield();

    acquire(lk);
}

// added by zcr
PRIVATE int ldt_seg_linear(PROCESS* p, int idx) {
    struct s_descriptor* d = &p->task.context.ldts[idx];
    return d->base_high << 24 | d->base_mid << 16 | d->base_low;
}

PUBLIC void* va2la(int pid, void* va) {
    if (kernel_initial == 1) { return va; }

    PROCESS* p = &proc_table[pid];
    u32 seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
    u32 la = seg_base + (u32)va;

    return (void*)la;
}

PUBLIC void idle() {
    while (1) {
        // disp_str("-idle-"); //mark debug
        rq_remove(p_proc_current);
        asm volatile("sti\n hlt" ::: "memory");
    }
    // p_proc_current->task.channel = 0;
}

/**
 * @brief 将进程状态置为睡眠，直至事件发生
 * added by zhenhao 2023.5.19
 * @param event:等待的事件
 * @return void
 */
PUBLIC void wait_event(void* event) {
    disable_int();
    p_proc_current->task.channel = event;
    p_proc_current->task.stat = SLEEPING;

    rq_remove(p_proc_current);
    enable_int();
    sched_yield();
}

/*
    added by cjjx 2021-12-25
*/
PUBLIC void wakeup_for_sem(void* chan) {
    wakeup(chan);
}

PRIVATE void stack_backtrace(u32 ebp) {
    disp_str("=>");
    disp_int(ebp);
    disp_str("(");
    disp_int(*(((u32*)ebp) + 1));
    disp_str(")\n");
}

PUBLIC void proc_backtrace() {
    u32 ebp;
    int skip = 2;
    disp_str("pid:");
    disp_int(proc2pid(p_proc_current));
    disp_str("\n");
    asm volatile("mov %%ebp, %0 " : "=r"(ebp));
    for (int i = 0; ebp && i < 8; i++) {
        if (i >= skip) { stack_backtrace(ebp); }
        ebp = *((u32*)ebp);
        if (ebp >= KernelLinMapBase || ebp < ShareLinLimitMAX) break;
    }
    while (1);
}
