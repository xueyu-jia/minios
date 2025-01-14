#include <minios/proc.h>
#include <minios/kstate.h>
#include <minios/interrupt.h>
#include <minios/asm.h>
#include <minios/console.h>
#include <minios/layout.h>
#include <minios/assert.h>
#include <string.h>
#include <stdbool.h>

u32 cr3_ready;

process_t* p_proc_current;
process_t* p_proc_next;
process_t cpu_table[NR_CPUS];
process_t proc_table[NR_PCBS];

extern void idle();
extern void task_tty();
extern void hd_service();
extern void bsync_service();

task_t task_table[NR_TASKS] = {
    {idle, 0, RPL_KERNEL, 0, PROC_NICE_MAX, STACK_SIZE_TASK, "task_idle"},
    {task_tty, 1, RPL_TASK, 1, 1, STACK_SIZE_TASK, "task_tty"},
    {hd_service, 1, RPL_TASK, 1, 2, STACK_SIZE_TASK, "hd_service"},
    //{bsync_service, 1, RPL_TASK, 1, 3, STACK_SIZE_TASK, "bsync"},
};

int kern_getpid() {
    return p_proc_current->task.pid;
}

int kern_getpid_by_name(const char* name) {
    for (process_t* proc = proc_table; proc < proc_table + NR_PCBS; ++proc) {
        if (strcmp(proc->task.p_name, name) == 0) { return proc->task.pid; }
    }
    return -1;
}

void kern_get_proc_msg(proc_msg* msg) {
    msg->pid = p_proc_current->task.pid;
    msg->nice = p_proc_current->task.nice;
    msg->sum_cpu_use = p_proc_current->task.sum_cpu_use;
    msg->vruntime = p_proc_current->task.vruntime;
}

static int ldt_seg_linear(process_t* p, int index) {
    const descriptor_t* desc = &p->task.context.ldts[index];
    return (desc->base2 << 24) | (desc->base1 << 16) | desc->base0;
}

void* va2la(int pid, void* va) {
    if (kstate_on_init) { return va; }

    process_t* p = &proc_table[pid];
    u32 seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
    u32 la = seg_base + (u32)va;

    return (void*)la;
}

void proc_backtrace() {
    u32 ebp;
    int skip = 2;
    kprintf("backtrace of process [pid=%d]:\n", proc2pid(p_proc_current));
    asm volatile("mov %%ebp, %0 " : "=r"(ebp));
    for (int i = 0; ebp && i < 8; ++i) {
        if (i >= skip) { kprintf("%*s=> 0x%p (%#x)", 4, "", ebp, ((u32*)ebp)[1]); }
        ebp = ((u32*)ebp)[0];
        if (ebp >= KernelLinMapBase || ebp < ShareLinLimitMAX) { break; }
    }
    while (true) { halt(); }
}
