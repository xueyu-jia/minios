
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <minios/buffer.h>
#include <minios/clock.h>
#include <minios/console.h>
#include <minios/const.h>
#include <minios/hd.h>
#include <minios/mmap.h>
#include <minios/page.h>
#include <minios/proc.h>
#include <minios/proto.h>
#include <minios/type.h>
#include <minios/kstate.h>
#include <minios/x86.h>
#include <string.h>

u32 cr3_ready;
int u_proc_sum;

PROCESS* p_proc_current;
PROCESS* p_proc_next;
PROCESS cpu_table[NR_CPUS];
PROCESS proc_table[NR_PCBS];
TASK task_table[NR_TASKS] = {
    {idle, 0, RPL_KERNEL, 0, PROC_NICE_MAX, STACK_SIZE_TASK, "task_idle"},
    {task_tty, 1, RPL_TASK, 1, 1, STACK_SIZE_TASK, "task_tty"},
    {hd_service, 1, RPL_TASK, 1, 2, STACK_SIZE_TASK, "hd_service"},
    // {bsync_service, 1, RPL_TASK, 1, 3,             STACK_SIZE_TASK, "bsync" }
};

static int ldt_seg_linear(PROCESS* p, int idx);

int kern_getpid() {
    return p_proc_current->task.pid;
}

// PUBLIC int do_getpid()
// {
//     return kern_getpid();
// }

// PUBLIC int sys_getpid()
// {
//     return do_getpid();
// }

int kern_getpid_by_name(char* name) {
    for (PROCESS* proc = proc_table; proc < proc_table + NR_PCBS; proc++) {
        if (strcmp(proc->task.p_name, name) == 0) { return proc->task.pid; }
    }
    return -1;
}

// PUBLIC int do_getpid_by_name(char* name)
// {
//     return kern_getpid_by_name(name);
// }

// PUBLIC int sys_getpid_by_name()
// {
//     return do_getpid_by_name((char*)get_arg(1));
// }

void kern_get_proc_msg(proc_msg* msg) {
    msg->pid = p_proc_current->task.pid;
    msg->nice = p_proc_current->task.nice;
    msg->sum_cpu_use = p_proc_current->task.sum_cpu_use;
    msg->vruntime = p_proc_current->task.vruntime;
}

void do_get_proc_msg(proc_msg* msg) {
    kern_get_proc_msg(msg);
}
void sys_get_proc_msg() {
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
    disable_int_begin();
    p_proc_current->task.channel = chan;
    p_proc_current->task.stat = SLEEPING;
    rq_remove(p_proc_current);
    release(lk);
    disable_int_end();

    sched_yield();

    acquire(lk);
}

// added by zcr
static int ldt_seg_linear(PROCESS* p, int index) {
    const descriptor_t* desc = &p->task.context.ldts[index];
    return (desc->base2 << 24) | (desc->base1 << 16) | desc->base0;
}

void* va2la(int pid, void* va) {
    if (kstate_on_init) { return va; }

    PROCESS* p = &proc_table[pid];
    u32 seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
    u32 la = seg_base + (u32)va;

    return (void*)la;
}

void idle() {
    while (1) {
        // kprintf("-idle-"); //mark debug
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
void wait_event(void* event) {
    disable_int_begin();
    p_proc_current->task.channel = event;
    p_proc_current->task.stat = SLEEPING;
    rq_remove(p_proc_current);
    disable_int_end();

    sched_yield();
}

/*
    added by cjjx 2021-12-25
*/
void wakeup_for_sem(void* chan) {
    wakeup(chan);
}

void proc_backtrace() {
    u32 ebp;
    int skip = 2;
    kprintf("backtrace of process [pid=%d]:\n", proc2pid(p_proc_current));
    asm volatile("mov %%ebp, %0 " : "=r"(ebp));
    for (int i = 0; ebp && i < 8; i++) {
        if (i >= skip) { kprintf("%*s=> 0x%p (%#x)", 4, "", ebp, ((u32*)ebp)[1]); }
        ebp = ((u32*)ebp)[0];
        if (ebp >= KernelLinMapBase || ebp < ShareLinLimitMAX) { break; }
    }
    while (true) { halt(); }
}
