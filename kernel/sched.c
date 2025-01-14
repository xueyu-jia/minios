#include <minios/sched.h>
#include <minios/buffer.h>
#include <minios/clock.h>
#include <minios/syscall.h>
#include <minios/console.h>
#include <minios/hd.h>
#include <minios/protect.h>
#include <minios/assert.h>
#include <minios/interrupt.h>
#include <minios/syscall.h>
#include <compiler.h>
#include <stdbool.h>
#include <string.h>

//! FIXME: assert in sched is not allowed, we need another special assert method
//! here
#define sched_assert(...) assert(__VA_ARGS__)

#define VRUNTIME_THRESHOULD 0xffffff

typedef struct sched_process_entity {
    u32 pid;
    process_t* p_process;
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

static u32 get_min_vruntime();
static sched_entity* get_current_entity(process_t* current_proc);
static void rq_resort(sched_entity* changed_entity);

/*======================================================================*
                              cfs_schedule
 *======================================================================*/

MAYBE_UNUSED static inline bool is_rq_empty() {
    sched_assert((rq == rq_tail) == (rq->next == NULL));
    return rq == rq_tail;
}
MAYBE_UNUSED static inline sched_entity* rq_front() {
    sched_assert(!is_rq_empty());
    return rq->next;
}
MAYBE_UNUSED static inline sched_entity* rq_back() {
    sched_assert(!is_rq_empty());
    return rq_tail;
}
MAYBE_UNUSED static inline process_t* rq_front_p() {
    return rq_front()->p_process;
}
MAYBE_UNUSED static inline process_t* rq_back_p() {
    return rq_back()->p_process;
}

MAYBE_UNUSED static inline bool is_rt_rq_empty() {
    sched_assert((rt_rq == rt_rq_tail) == (rt_rq->next == NULL));
    return rt_rq == rt_rq_tail;
}
MAYBE_UNUSED static inline sched_entity* rt_rq_front() {
    sched_assert(!is_rt_rq_empty());
    return rt_rq->next;
}
MAYBE_UNUSED static inline sched_entity* rt_rq_back() {
    sched_assert(!is_rt_rq_empty());
    return rt_rq_tail;
}
MAYBE_UNUSED static inline process_t* rt_rq_front_p() {
    return rt_rq_front()->p_process;
}
MAYBE_UNUSED static inline process_t* rt_rq_back_p() {
    return rt_rq_back()->p_process;
}

static inline void consume_proc_cpu_time(process_t* proc) {
    if (!proc->task.is_rt) {
        proc->task.vruntime += proc->task.cpu_use * 1024. / proc->task.weight;
    }
    proc->task.sum_cpu_use += proc->task.cpu_use;
    proc->task.cpu_use = 0;
}

void cfs_sched() {
    bool need_maintain = false;

    process_t* sched_next = NULL;
    do {
#define SCHED_NEXT(next)        \
    assert(sched_next == NULL); \
    sched_next = (next);        \
    break;

        const auto cur = &p_proc_current->task;

        const bool expect_realtime_next = rt_runtime < sysctl_sched_rt_runtime && !is_rt_rq_empty();
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
            const int pid = kern_getpid_by_name("task_idle");
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
    for (int i = 0; i < PROC_READY_MAX; ++i) {
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
sched_entity* get_current_entity(process_t* current_proc) {
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

static sched_entity* find_new_sched_entity(sched_entity* array) {
    for (int i = 0; i < PROC_READY_MAX; ++i) {
        if (array[i].pid == PID_NO_PROC) { return &array[i]; }
    }
    return NULL;
}

void rq_insert(process_t* proc) {
    int is_rt = proc->task.is_rt;
    sched_entity* _rq_head = (is_rt) ? rt_rq : rq;
    sched_entity* _rq_tail = (is_rt) ? rt_rq_tail : rq_tail;
    sched_entity* _rq_arr = (is_rt) ? rt_rq_array : rq_array;
    sched_entity* new_entity = find_new_sched_entity(_rq_arr);
    if (new_entity == NULL) {
        kprintf("in_rq error1: cannot find a rq_array\n");
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
            while (pos->next != NULL && pos->next->p_process->task.rt_priority >
                                            new_entity->p_process->task.rt_priority) {
                pos = pos->next;
            }
        } else {
            while (pos->next != NULL &&
                   pos->next->p_process->task.vruntime <= new_entity->p_process->task.vruntime) {
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

void rq_remove(process_t* proc) {
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

void kern_yield() {
    p_proc_current->task.vruntime += 5;
    sched();
}

void sched_yield() {
    if (get_ring_level() == RPL_KERNEL) {
        kern_yield();
    } else {
        syscall(NR_yield);
    }
}

void kern_sleep(int ms) {
    int ticks0 = ticks;
    while (ticks - ticks0 < ms) { wait_event(&ticks); }
}

void wakeup(void* channel) {
    disable_int_begin();

    process_t* candidates[NR_PCBS] = {};
    process_t** slot = candidates;

    for (int i = 0; i < NR_PCBS; ++i) {
        const auto proc = &proc_table[i];
        if (proc->task.stat == SLEEPING && proc->task.channel == channel) { *slot++ = proc; }
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
void kern_nice(int inc) {
    if (p_proc_current->task.is_rt == true) return;

    inc = CLAMP(inc, -20, 19);
    p_proc_current->task.nice = CLAMP(p_proc_current->task.nice + inc, -20, 19);
    p_proc_current->task.weight = nice_to_weight[p_proc_current->task.nice + 20];
}

void kern_set_rt(bool turn_rt) {
    if (!!turn_rt == !!p_proc_current->task.is_rt) { return; }
    rq_remove(p_proc_current);
    p_proc_current->task.is_rt = turn_rt;
    rq_insert(p_proc_current);
    sched();
}

void kern_rt_prio(int prio) {
    if (prio < 0 || p_proc_current->task.is_rt == false)
        return;
    else { p_proc_current->task.rt_priority = prio; }
    sched();
}

// Atomically spinlock_release lock and sleep on chan.
// Respinlock_acquires lock when awakened.
void wait_for_sem(void* chan, spinlock_t* lock) {
    assert(p_proc_current != NULL);
    assert(lock != NULL);

    disable_int_begin();
    p_proc_current->task.channel = chan;
    p_proc_current->task.stat = SLEEPING;
    rq_remove(p_proc_current);
    spinlock_release(lock);
    disable_int_end();

    sched_yield();

    spinlock_acquire(lock);
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

void wakeup_for_sem(void* chan) {
    wakeup(chan);
}
