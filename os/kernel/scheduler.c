/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-15 09:42:23
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-15 09:49:06
 * @FilePath: /minios/os/kernel/scheduler.c
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include <kernel/buffer.h>
#include <kernel/clock.h>
#include <kernel/console.h>
#include <kernel/const.h>
#include <kernel/hd.h>
#include <kernel/proc.h>
#include <kernel/protect.h>
#include <kernel/proto.h>
#include <kernel/type.h>
#include <klib/assert.h>
#include <klib/compiler.h>
#include <klib/string.h>

//! FIXME: assert in sched is not allowed, we need another special assert method
//! here
#define sched_assert(...) assert(__VA_ARGS__)

#define VRUNTIME_THRESHOULD 0xffffff

typedef struct sched_process_entity {
    u32 pid;
    PROCESS* p_process;
    struct sched_process_entity* next;
    struct sched_process_entity* prev;
    // int pri;
} sched_entity;

int nice_to_weight[40] = {
    /* -20 */ 88761, 71755, 56483, 46273, 36291,
    /* -15 */ 29154, 23254, 18705, 14949, 11916,
    /* -10 */ 9548,  7620,  6100,  4904,  3906,
    /*  -5 */ 3121,  2501,  1991,  1586,  1277,
    /*   0 */ 1024,  820,   655,   526,   423,
    /*   5 */ 335,   272,   215,   172,   137,
    /*  10 */ 110,   87,    70,    56,    45,
    /*  15 */ 36,    29,    23,    18,    15};

//! realtime runtime queue
sched_entity rt_rq_head_empty = {-1, NULL, NULL, NULL};
sched_entity* rt_rq = &rt_rq_head_empty;
sched_entity* rt_rq_tail = &rt_rq_head_empty;
sched_entity rt_rq_array[PROC_READY_MAX];

//! non-realtime runtime queue
sched_entity rq_head_empty = {-1, NULL, NULL, NULL};
sched_entity* rq = &rq_head_empty;
sched_entity* rq_tail = &rq_head_empty;
sched_entity rq_array[PROC_READY_MAX];

int sysctl_sched_rt_period = 10;
int sysctl_sched_rt_runtime = 9;
int rt_runtime;
int rt_period;

PRIVATE u32 get_min_vruntime();
PRIVATE sched_entity* get_current_entity(PROCESS* current_proc);
PRIVATE void rq_resort(sched_entity* changed_entity);

/*======================================================================*
                              cfs_schedule
 *======================================================================*/

static inline bool is_rq_empty() {
    sched_assert((rq == rq_tail) == (rq->next == NULL));
    return rq == rq_tail;
}
static inline sched_entity* rq_front() {
    sched_assert(!is_rq_empty());
    return rq->next;
}
static inline sched_entity* rq_back() {
    sched_assert(!is_rq_empty());
    return rq_tail;
}
static inline PROCESS* rq_front_p() {
    return rq_front()->p_process;
}
static inline PROCESS* rq_back_p() {
    return rq_back()->p_process;
}

static inline bool is_rt_rq_empty() {
    sched_assert((rt_rq == rt_rq_tail) == (rt_rq->next == NULL));
    return rt_rq == rt_rq_tail;
}
static inline sched_entity* rt_rq_front() {
    sched_assert(!is_rt_rq_empty());
    return rt_rq->next;
}
static inline sched_entity* rt_rq_back() {
    sched_assert(!is_rt_rq_empty());
    return rt_rq_tail;
}
static inline PROCESS* rt_rq_front_p() {
    return rt_rq_front()->p_process;
}
static inline PROCESS* rt_rq_back_p() {
    return rt_rq_back()->p_process;
}

static inline void consume_proc_cpu_time(PROCESS* proc) {
    if (!proc->task.is_rt) {
        proc->task.vruntime += proc->task.cpu_use * 1024. / proc->task.weight;
    }
    proc->task.sum_cpu_use += proc->task.cpu_use;
    proc->task.cpu_use = 0;
}

void cfs_sched() {
    bool need_maintain = false;

    PROCESS* sched_next = NULL;
    do {
#define SCHED_NEXT(next)        \
    assert(sched_next == NULL); \
    sched_next = (next);        \
    break;

        const auto cur = &p_proc_current->task;

        const bool expect_realtime_next =
            rt_runtime < sysctl_sched_rt_runtime && !is_rt_rq_empty();
        if (expect_realtime_next && cur->is_rt && cur->stat == READY &&
            cur->rt_priority > rt_rq_front_p()->task.rt_priority) {
            SCHED_NEXT(p_proc_current);
        }

        consume_proc_cpu_time(p_proc_current);

        if (expect_realtime_next) {
            if (!cur->is_rt) {
                auto cur_ent = get_current_entity(p_proc_current);
                if (cur_ent != NULL && cur_ent->next != NULL &&
                    cur->vruntime > cur_ent->next->p_process->task.vruntime) {
                    rq_resort(cur_ent);
                }
            }
            SCHED_NEXT(rt_rq_front_p());
        }

        if (is_rq_empty()) {
            const int pid = kern_get_pid_byname("task_idle");
            sched_assert(pid != -1);
            auto idle = &proc_table[pid];
            idle->task.stat = READY;
            rq_insert(idle);
            sched_assert(rq_front_p() == idle);
            SCHED_NEXT(rq_front_p());
        }

        if (cur->is_rt || cur->stat != READY) { SCHED_NEXT(rq_front_p()); }

        //! ATTENTION: i'm not sure whether cur_ent must be non null here
        //! @zymelaii
        auto cur_ent = get_current_entity(p_proc_current);
        if (cur_ent != NULL && cur_ent->next != NULL &&
            cur->vruntime > cur_ent->next->p_process->task.vruntime) {
            rq_resort(cur_ent);
        }

        need_maintain = true;

        SCHED_NEXT(rq_front_p());
#undef SCHED_NEXT
    } while (0);

    /*!
     * two purpose for maintaining procedure
     *
     * 1. avoid vruntime overflow
     * 2. use a threshould to avoid frequent maintaing
     */
    if (need_maintain) {
        const int threshold = VRUNTIME_THRESHOULD;
        if (!is_rq_empty() && rq_back_p()->task.vruntime > threshold) {
            const double min_vruntime = get_min_vruntime();
            auto ent = rq_front();
            while (ent != NULL) {
                ent->p_process->task.vruntime -= min_vruntime;
                ent = ent->next;
            }
        }
    }

    sched_assert(sched_next != NULL);
    p_proc_next = sched_next;
}

void sched_init() {
    // added by zq
    for (int i = 0; i < PROC_READY_MAX; i++) {
        rt_rq_array[i].pid = PID_NO_PROC;
        rq_array[i].pid = PID_NO_PROC;
    }
}

void proc_update() {
    ++rt_period;

    //! FIXME: 关于下面这两段的执行顺序是否有问题暂且存疑

    if (p_proc_current->task.is_rt) { ++rt_runtime; }

    if (rt_period > sysctl_sched_rt_period) {
        rt_period = 0;
        rt_runtime = 0;
    }

    if (!p_proc_current->task.is_rt) { ++p_proc_current->task.cpu_use; }
}

void schedule() {
    cfs_sched();
    cr3_ready = p_proc_next->task.cr3;
}

u32 get_min_vruntime() {
    sched_entity* pos = rq->next;
    u32 min_vruntime = VRUNTIME_THRESHOULD;
    while (pos != NULL) {
        if (min_vruntime > pos->p_process->task.vruntime) {
            min_vruntime = pos->p_process->task.vruntime;
        }
        pos = pos->next;
    }
    return min_vruntime;
}

// 返回指定PCB的调度实体sched_entity
sched_entity* get_current_entity(PROCESS* current_proc) {
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
    changed_entity->pid = PID_NO_PROC; // mark empty
    changed_entity->prev->next = changed_entity->next;
    if (changed_entity->next != NULL) {
        changed_entity->next->prev = changed_entity->prev;
    } else {
        rq_tail = changed_entity->prev;
    }

    rq_insert(changed_entity->p_process); // reinsert
}

PRIVATE sched_entity* find_new_sched_entity(sched_entity* array) {
    for (int i = 0; i < PROC_READY_MAX; ++i) {
        if (array[i].pid == PID_NO_PROC) { return &array[i]; }
    }
    return NULL;
}

void rq_insert(PROCESS* proc) {
    int is_rt = proc->task.is_rt;
    sched_entity* _rq_head = (is_rt) ? rt_rq : rq;
    sched_entity* _rq_tail = (is_rt) ? rt_rq_tail : rq_tail;
    sched_entity* _rq_arr = (is_rt) ? rt_rq_array : rq_array;
    sched_entity* new_entity = find_new_sched_entity(_rq_arr);
    if (new_entity == NULL) {
        disp_str("in_rq error1: cannot find a rq_array\n");
        return; // mark 这里没做错误处理和提示，之后要加上
    }
    new_entity->pid = proc->task.pid;
    new_entity->p_process = proc;
    new_entity->next = NULL;
    new_entity->prev = NULL;

    // new_entity.pri=p_in->task.rt_priority;
    if (_rq_head == _rq_tail) // rq is empty
    {
        _rq_head->next = new_entity;
        new_entity->prev = _rq_head;
        _rq_tail = new_entity;
    } else // not empty,find propery position
    {
        sched_entity* pos = _rq_head;
        // avoid re_in_rq
        if (is_rt) {
            while (pos->next != NULL &&
                   pos->next->p_process->task.rt_priority >
                       new_entity->p_process->task.rt_priority) {
                pos = pos->next;
            }
        } else {
            while (pos->next != NULL &&
                   pos->next->p_process->task.vruntime <=
                       new_entity->p_process->task.vruntime) {
                pos = pos->next;
            }
        }
        if (pos->next == NULL) {
            pos->next = new_entity;
            new_entity->prev = pos;
            _rq_tail = new_entity;
        } else {
            new_entity->prev = pos;
            new_entity->next = pos->next;
            pos->next->prev = new_entity;
            pos->next = new_entity;
        }
    }
    if (is_rt) {
        rt_rq_tail = _rq_tail;
    } else {
        rq_tail = _rq_tail;
    }
}

void rq_remove(PROCESS* proc) {
    auto ent = proc->task.is_rt ? rt_rq_front() : rq_front();
    while (ent != NULL && ent->pid != proc->task.pid) { ent = ent->next; }
    if (ent == NULL) { return; }
    ent->prev->next = ent->next;
    if (ent->next != NULL) {
        ent->next->prev = ent->prev;
    } else if (proc->task.is_rt) {
        rt_rq_tail = ent->prev;
    } else {
        rq_tail = ent->prev;
    }
    ent->next = NULL;
    ent->prev = NULL;
    ent->pid = PID_NO_PROC;
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

PUBLIC void sched_yield() {
    if (get_ring_level() == RPL_KERNEL) {
        kern_yield();
    } else {
        yield();
    }
}

PUBLIC void do_yield() {
    kern_yield();
}

PUBLIC void sys_yield() {
    do_yield();
}

PUBLIC void kern_sleep(int ms) {
    int ticks0 = ticks;
    while (ticks - ticks0 < ms) { wait_event(&ticks); }
}

PUBLIC void do_sleep(int ms) {
    return kern_sleep(ms);
}

PUBLIC void sys_sleep() {
    return do_sleep(get_arg(1));
}

PUBLIC void wakeup(void* channel) {
    disable_int_begin();

    PROCESS* candidates[NR_PCBS] = {};
    PROCESS** slot = candidates;

    for (int i = 0; i < NR_PCBS; ++i) {
        const auto proc = &proc_table[i];
        if (proc->task.stat == SLEEPING && proc->task.channel == channel) {
            *slot++ = proc;
        }
    }

    //! ATTENTION: the reason why the following method works is because size of
    //! rq equals to max pcbs
    //! FIXME: a completely concurrency-unsafe impl
    //! FIXME: expect a queue or other impl rather than simply wakeup all

    const auto min_vruntime = get_min_vruntime();
    for (auto p = candidates; p < slot; ++p) {
        auto proc = *p;
        proc->task.stat = READY;
        proc->task.channel = NULL;
        if (!proc->task.is_rt) { proc->task.vruntime = min_vruntime; }
        rq_insert(proc);
    }

    disable_int_end();
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

PUBLIC void do_nice(int inc) {
    return kern_nice(inc);
}

PUBLIC void sys_nice() {
    do_nice(get_arg(1));
}

PUBLIC void kern_set_rt(int turn_rt) {
    if (turn_rt == true &&
        p_proc_current->task.is_rt == false) // not rt change to rt process
    {
        rq_remove(p_proc_current);
        p_proc_current->task.is_rt = true;
    } else if (turn_rt == false && p_proc_current->task.is_rt ==
                                       true) // rt change to not rt process
    {
        rq_remove(p_proc_current);
        p_proc_current->task.is_rt = false;
    } else {
        return;
    }
    rq_insert(p_proc_current);
    sched();
}

PUBLIC void do_set_rt(int turn_rt) {
    kern_set_rt(turn_rt);
}

PUBLIC void sys_set_rt() {
    do_set_rt(get_arg(1));
}

PUBLIC void kern_rt_prio(int prio) {
    if (prio < 0 || p_proc_current->task.is_rt == false)
        return;
    else { p_proc_current->task.rt_priority = prio; }
    sched();
}

PUBLIC void do_rt_prio(int prio) {
    kern_rt_prio(prio);
}
PUBLIC void sys_rt_prio() {
    do_rt_prio(get_arg(1));
}
