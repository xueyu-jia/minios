
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "clock.h"
#include "console.h"
#include "proc.h"
#include "proto.h"
#include "hd.h"
#include "buffer.h"

// process entity
typedef struct sched_process_entity {
    u32                          pid;
    PROCESS*                     p_process;
    struct sched_process_entity* next;
    struct sched_process_entity* prev;
    // int pri;
} sched_entity;

u32            cr3_ready;
int            u_proc_sum;
int            kernel_initial;
PROCESS*       p_proc_current;
PROCESS*       p_proc_next;
PUBLIC PROCESS cpu_table[NR_CPUS];
PUBLIC PROCESS proc_table[NR_PCBS];
PUBLIC TASK    task_table[NR_TASKS] = {
    {idle,          0, RPL_KRNL, 0, PROC_NICE_MAX, STACK_SIZE_TASK, "task_idle" },
    {task_tty,      1, RPL_TASK, 1, 1,             STACK_SIZE_TASK, "task_tty"  },
    {hd_service,    1, RPL_TASK, 1, 2,             STACK_SIZE_TASK, "hd_service"},
    // {bsync_service, 1, RPL_TASK, 1, 3,             STACK_SIZE_TASK, "bsync"     }
};

// added by zq
int nice_to_weight[40] = {
    /* -20 */ 88761, 71755, 56483, 46273, 36291,
    /* -15 */ 29154, 23254, 18705, 14949, 11916,
    /* -10 */ 9548,  7620,  6100,  4904,  3906,
    /*  -5 */ 3121,  2501,  1991,  1586,  1277,
    /*   0 */ 1024,  820,   655,   526,   423,
    /*   5 */ 335,   272,   215,   172,   137,
    /*  10 */ 110,   87,    70,    56,    45,
    /*  15 */ 36,    29,    23,    18,    15};

sched_entity  head1      = {-1, NULL, NULL, NULL};
sched_entity* rt_rq      = &head1;
sched_entity* rt_rq_tail = &head1;
sched_entity  rt_rq_array[PROC_READY_MAX];

sched_entity  head2   = {-1, NULL, NULL, NULL};
sched_entity* rq      = &head2;
sched_entity* rq_tail = &head2;
sched_entity  rq_array[PROC_READY_MAX];

int sysctl_sched_rt_period  = 10;
int sysctl_sched_rt_runtime = 9;
int rt_runtime;
int rt_period;

u32           get_min_vruntime();
sched_entity* get_curr_entity(PROCESS* current_proc);
void          rq_resort(sched_entity* changed_entity);

PRIVATE int test_cfs() {
    if (p_proc_current->task.stat == READY
        || rt_rq->next->p_process->task.stat == READY)
        return true;
    else
        return false;
}

/*======================================================================*
                              cfs_schedule
 *======================================================================*/

void cfs_sched() {
    if (rt_runtime < sysctl_sched_rt_runtime
        && rt_rq->next
               != NULL) // rt_runtime<950000 rt_rq is not empty(静态优先级+FIFO)
    {
        if (p_proc_current->task.is_rt == true) {
            if (p_proc_current->task.stat == READY
                && p_proc_current->task.rt_priority
                       > rt_rq->next->p_process->task.rt_priority) {
                p_proc_next = p_proc_current;
            } else {
                p_proc_current->task.sum_cpu_use +=
                    p_proc_current->task.cpu_use;
                p_proc_current->task.cpu_use = 0;

                p_proc_next = rt_rq->next->p_process;
            }
        } else {
            p_proc_current->task.vruntime += p_proc_current->task.cpu_use * 1024
                                           / p_proc_current->task.weight;
            p_proc_current->task.sum_cpu_use += p_proc_current->task.cpu_use;
            p_proc_current->task.cpu_use      = 0;

            sched_entity* curr_entity = get_curr_entity(p_proc_current);

            if (curr_entity != NULL && curr_entity->next != NULL
                && p_proc_current->task.vruntime
                       > curr_entity->next->p_process->task.vruntime) {
                // resort
                rq_resort(curr_entity);
            }

            p_proc_next = rt_rq->next->p_process;
        }
    } else {
        // update vruntime/sum_cpu_use
        if (p_proc_current->task.is_rt == false) {
            p_proc_current->task.vruntime += p_proc_current->task.cpu_use
                                           * (double)1024
                                           / p_proc_current->task.weight;
        }
        p_proc_current->task.sum_cpu_use += p_proc_current->task.cpu_use;
        p_proc_current->task.cpu_use      = 0;

        if (rq->next == NULL) {
            // disp_str("-cfs-");	//mark debug
            int idle_pid                   = kern_get_pid_byname("task_idle");
            proc_table[idle_pid].task.stat = READY; // IDLE
            in_rq(&proc_table[idle_pid]);
            p_proc_next = rq->next->p_process;
            return;
        }
        // disp_int(rq->next->p_process->task.pid);

        if (p_proc_current->task.stat != READY
            || p_proc_current->task.is_rt == true) {
            p_proc_next = rq->next->p_process;
        } else {
            sched_entity* curr_entity = get_curr_entity(p_proc_current);
            // if(curr_entity == NULL){
            // 	disp_str("cfs_sched error1: cannot find the curr_entity");
            // 	while(1){};
            // }
            if (curr_entity != NULL && curr_entity->next != NULL
                && p_proc_current->task.vruntime
                       > curr_entity->next->p_process->task.vruntime) {
                // resort
                rq_resort(curr_entity);
            }
            p_proc_next = rq->next->p_process;

            if (rq_tail->p_process->task.vruntime
                > 0xFFFFF) // vruntime-min_vruntime,avoid overflow and time
                           // waste
            {
                sched_entity* pos          = rq->next;
                double        min_vruntime = get_min_vruntime();

                while (pos != NULL) {
                    pos->p_process->task.vruntime -= min_vruntime;
                    pos                            = pos->next;
                }
            }
        }
    }
}

void sched_init() {
    // added by zq
    for (int i = 0; i < PROC_READY_MAX; i++) {
        rt_rq_array[i].pid = PID_NO_PROC;
        rq_array[i].pid    = PID_NO_PROC;
    }
}

void proc_update() {
    rt_period++;
    if (p_proc_current->task.is_rt == true) { rt_runtime++; }

    if (rt_period > sysctl_sched_rt_period) // initialize
    {
        rt_period = rt_runtime = 0;
    }

    if (p_proc_current->task.is_rt == false) { p_proc_current->task.cpu_use++; }
    // p_proc_current->task.ticks--;
}

void schedule() {
    cfs_sched();
    cr3_ready = p_proc_next->task.cr3;
}

u32 get_min_vruntime() {
    sched_entity* pos          = rq->next;
    u32           min_vruntime = 0xFFFFFF;

    while (pos != NULL) {
        if (min_vruntime > pos->p_process->task.vruntime) {
            min_vruntime = pos->p_process->task.vruntime;
        }
        pos = pos->next;
    }
    return min_vruntime;
}

// 返回指定PCB的调度实体sched_entity
sched_entity* get_curr_entity(PROCESS* current_proc) {
    sched_entity* pos = rq->next;

    while (pos != NULL) {
        if (pos->p_process == current_proc) { return pos; }
        pos = pos->next;
    }
    return NULL;
}

// 删除指定节点（但没释放该节点的空间，只是用pid=PID_NO_PROC标空了）
// 并将该节点的PCB重新插入链表
void rq_resort(sched_entity* changed_entity) // only for rq
{
    changed_entity->pid        = PID_NO_PROC; // mark empty
    changed_entity->prev->next = changed_entity->next;
    if (changed_entity->next != NULL) {
        changed_entity->next->prev = changed_entity->prev;
    } else {
        rq_tail = changed_entity->prev;
    }

    in_rq(changed_entity->p_process); // reinsert
}

void in_rq(PROCESS* p_in) {
    if (p_in->task.is_rt == true) // real time process
    {
        int           i = 0;
        sched_entity* new_entity;

        for (; i < PROC_READY_MAX && rt_rq_array[i].pid < NR_PCBS; i++)
            ;
        if (i < PROC_READY_MAX) {
            new_entity = &rt_rq_array[i];
        } else {
            disp_str("in_rq error1: cannot find a rq_array\n");
            return; // mark 这里没做错误处理和提示，之后要加上
        }
        new_entity->pid       = p_in->task.pid;
        new_entity->p_process = p_in;
        new_entity->next      = NULL;
        new_entity->prev      = NULL;

        // new_entity.pri=p_in->task.rt_priority;
        if (rt_rq == rt_rq_tail) // rq is empty
        {
            rt_rq->next      = new_entity;
            new_entity->prev = rt_rq;
            rt_rq_tail       = new_entity;
        } else // not empty,find propery position
        {
            sched_entity* pos = rt_rq->next;
            // avoid re_in_rq
            pos = rt_rq;
            while (pos->next != NULL
                   && pos->next->p_process->task.rt_priority
                          > new_entity->p_process->task.rt_priority) {
                pos = pos->next;
            }
            if (pos->next == NULL) {
                pos->next        = new_entity;
                new_entity->prev = pos;
                rt_rq_tail       = new_entity;
            } else {
                new_entity->prev = pos;
                new_entity->next = pos->next;
                pos->next->prev  = new_entity;
                pos->next        = new_entity;
            }
        }

    } else // not real time process
    {
        int           i = 0;
        sched_entity* new_entity;

        for (; i < PROC_READY_MAX && rq_array[i].pid < NR_PCBS; i++)
            ;
        if (i < PROC_READY_MAX) {
            new_entity = &rq_array[i];
        } else {
            disp_str("in_rq error2: cannot find a rq_array\n");
            return; // mark 这里没做错误处理和提示，之后要加上
        }
        new_entity->pid       = p_in->task.pid;
        new_entity->p_process = p_in;
        new_entity->next      = NULL;
        new_entity->prev      = NULL;

        // new_entity.pri=p_in->task.vruntime;
        if (rq == rq_tail) // rq is empty
        {
            rq->next         = new_entity;
            new_entity->prev = rq;
            rq_tail          = new_entity;
        } else // not empty,find propery position
        {
            sched_entity* pos = rq->next;
            // avoid re_in_rq
            pos = rq;
            while (pos->next != NULL
                   && pos->next->p_process->task.vruntime
                          <= new_entity->p_process->task.vruntime)
                pos = pos->next;
            if (pos->next == NULL) {
                pos->next        = new_entity;
                new_entity->prev = pos;
                rq_tail          = new_entity;
            } else {
                new_entity->prev = pos;
                new_entity->next = pos->next;
                pos->next->prev  = new_entity;
                pos->next        = new_entity;
            }
        }
    }
}

void out_rq(PROCESS* p_out) {
    sched_entity* pos;
    if (p_out->task.is_rt == true) // real time process
    {
        pos = rt_rq->next;
    } else // not real time process
    {
        pos = rq->next;
    }
    while (pos != NULL && pos->pid != p_out->task.pid) pos = pos->next;
    if (pos != NULL) // found
    {
        pos->prev->next = pos->next;
        if (pos->next != NULL) {
            pos->next->prev = pos->prev;
        } else {
            if (p_out->task.is_rt == true) // real time process
            {
                rt_rq_tail = pos->prev;
            } else // not real time process
            {
                rq_tail = pos->prev;
            }
        }
        pos->next = NULL;
        pos->prev = NULL;
        pos->pid  = PID_NO_PROC; // mark empty
    }
}

/*======================================================================*
                           alloc_PCB  add by visual 2016.4.8
 *======================================================================*/
PUBLIC PROCESS* alloc_PCB() { // 分配PCB表
    PROCESS* p;
    int      i;
    p = proc_table + NR_K_PCBS; // 跳过前NR_K_PCBS个
    for (i = NR_K_PCBS; i < NR_PCBS; i++) {
        if (p->task.stat == FREE)
            break; // FREE表示当前PCB是空闲的, modified by mingxuan 2021-8-21
        p++;
    }
    if (i >= NR_PCBS)
        return 0; // NULL
    else
        return p;
}

/*======================================================================*
                           free_PCB  add by visual 2016.4.8
 *======================================================================*/
PUBLIC void free_PCB(PROCESS* p) { // 释放PCB表
    p->task.stat = FREE; 
    // FREE表示当前PCB是空闲的, modified by mingxuan 2021-8-21
}

/*======================================================================*
                           get_pid		add by visual 2016.4.6
 *======================================================================*/
PUBLIC int sys_get_pid() {
    return do_get_pid();
}

PUBLIC int do_get_pid() {
    return kern_get_pid();
}

// added by mingxuan 2021-8-14
PUBLIC int kern_get_pid() {
    return p_proc_current->task.pid;
}

PUBLIC int sys_get_pid_byname() {
    return do_get_pid_byname(get_arg(1));
}

PUBLIC int do_get_pid_byname(char* name) {
    return kern_get_pid_byname(name);
}

PUBLIC int kern_get_pid_byname(char* name) {
    for (PROCESS* proc = proc_table; proc < proc_table + NR_PCBS; proc++) {
        if (strcmp(proc->task.p_name, name) == 0) {
            return proc->task.pid; 
        }
    }
    return -1;
}

/*======================================================================*
                           yield and sleep
 *======================================================================*/
// used for processes to give up the CPU
PUBLIC void kern_yield() {
    // p_proc_current->task.ticks = 0;	/* modified by xw, 18/4/27 */
    p_proc_current->task.vruntime += 5;
    sched(); // Modified by xw, 18/4/19
}

PUBLIC void do_yield() {
    kern_yield();
}

PUBLIC void sys_yield() {
    do_yield();
}

PUBLIC void kern_sleep(int n) {
    int ticks0;

    ticks0                       = ticks;
    p_proc_current->task.channel = &ticks;

    while (ticks - ticks0 < n) {
        p_proc_current->task.stat = SLEEPING;
        out_rq(p_proc_current);
        sched(); // Modified by xw, 18/4/19
    }
    p_proc_current->task.channel = 0;
}

PUBLIC void do_sleep(int n) {
    return kern_sleep(n);
}

PUBLIC void sys_sleep() {
    return do_sleep(get_arg(1));
}

/*invoked by clock-interrupt handler to wakeup
 *processes sleeping on ticks.
 */
PUBLIC void wakeup(void* channel) {
    PROCESS *p, *target_p;
    target_p = NULL;

    for (p = proc_table; p < proc_table + NR_PCBS; p++) {
        if (p->task.stat == SLEEPING && p->task.channel == channel) {
            p->task.stat = READY;
            target_p     = p;
            break;
        }
    }
    if (target_p != NULL) {
        if (target_p->task.is_rt == false) {
            target_p->task.vruntime = get_min_vruntime(); // min_vruntime
        }
        target_p->task.channel = 0;
        in_rq(target_p);
    }
}

PUBLIC void sys_nice() {
    do_nice(get_arg(1));
}

PUBLIC void do_nice(int inc) {
    return kern_nice(inc);
}

// added by zq
PUBLIC void kern_nice(int inc) {
    if (p_proc_current->task.is_rt == true) return;

    if (inc < -20)
        inc = -20;
    else if (inc > 19)
        inc = 19;

    if (inc + p_proc_current->task.nice < -20) {
        p_proc_current->task.nice = -20;
    } else if (inc + p_proc_current->task.nice > 19) {
        p_proc_current->task.nice = 19;
    } else {
        p_proc_current->task.nice += inc;
    }
    p_proc_current->task.weight =
        nice_to_weight[p_proc_current->task.nice + 20];
}

// void sys_exit()
// {
// 	p_proc_current->task.stat = IDLE;
// 	out_rq(p_proc_current);
// 	sched();
// }

PUBLIC void sys_set_rt() {
    do_set_rt(get_arg(1));
}

PUBLIC void do_set_rt(int turn_rt) {
    kern_set_rt(turn_rt);
}

PUBLIC void kern_set_rt(int turn_rt) {
    if (turn_rt == true
        && p_proc_current->task.is_rt == false) // not rt change to rt process
    {
        out_rq(p_proc_current);
        p_proc_current->task.is_rt = true;
    } else if (
        turn_rt == false
        && p_proc_current->task.is_rt == true) // rt change to not rt process
    {
        out_rq(p_proc_current);
        p_proc_current->task.is_rt = false;
    } else {
        return;
    }
    in_rq(p_proc_current);
    sched();
}

PUBLIC void sys_rt_prio() {
    do_rt_prio(get_arg(1));
}

PUBLIC void do_rt_prio(int prio) {
    kern_rt_prio(prio);
}

PUBLIC void kern_rt_prio(int prio) {
    if (prio < 0 || p_proc_current->task.is_rt == false)
        return;
    else { p_proc_current->task.rt_priority = prio; }
    sched();
}

PUBLIC void sys_get_proc_msg() {
    do_get_proc_msg(get_arg(1));
}

PUBLIC void do_get_proc_msg(proc_msg* msg) {
    kern_get_proc_msg(msg);
}

PUBLIC void kern_get_proc_msg(proc_msg* msg) {
    msg->pid         = p_proc_current->task.pid;
    msg->nice        = p_proc_current->task.nice;
    msg->sum_cpu_use = p_proc_current->task.sum_cpu_use;
    msg->vruntime    = p_proc_current->task.vruntime;
}

/*
    added by cjjx 2021-12-25
*/
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
    p_proc_current->task.channel = chan;
    p_proc_current->task.stat    = SLEEPING;
    out_rq(p_proc_current);

    release(lk);

    u32 ringlevel = get_ring_level();
    if (ringlevel == 0) {
        sched();
    } else {
        yield();
    }

    acquire(lk);
}

/*
    added by cjjx 2021-12-25
*/
void wakeup_for_sem(void* chan) {
    wakeup(chan);
}

// added by zcr
PUBLIC int ldt_seg_linear(PROCESS* p, int idx) {
    struct s_descriptor* d = &p->task.ldts[idx];
    return d->base_high << 24 | d->base_mid << 16 | d->base_low;
}

PUBLIC void* va2la(int pid, void* va) {
    if (kernel_initial == 1) { return va; }

    PROCESS* p        = &proc_table[pid];
    u32      seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
    u32      la       = seg_base + (u32)va;

    return (void*)la;
}

/**
 * @brief 将进程状态置为睡眠，直至事件发生
 * added by zhenhao 2023.5.19
 * @param event:等待的事件
 * @return void
 */
PUBLIC void wait_event(void* event) {
    p_proc_current->task.channel = event;
    p_proc_current->task.stat    = SLEEPING;

    out_rq(p_proc_current);
    u32 ringlevel = get_ring_level();
    if (ringlevel == 0) {
        sched(); // 非系统调用（sched中有为cr3赋值的操作，只有ring0级别才能执行）
    } else {
        yield(); // 系统调用
    }
}

PUBLIC void idle() {
    while (1) {
        // disp_str("-idle-"); //mark debug
        out_rq(p_proc_current);
		asm volatile ("sti\n hlt" :::"memory");
    }
    // p_proc_current->task.channel = 0;
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
    asm volatile("mov %%ebp, %0 " : "=r"(ebp));
    for (int i = 0; ebp && i < 8; i++) {
        stack_backtrace(ebp);
        ebp = *((u32*)ebp);
        if(ebp >= KernelLinMapBase || ebp < StackLinLimitMAX)break;
    }
    while (1)
        ;
}