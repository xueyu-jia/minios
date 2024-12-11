#ifndef USER_PTHREAD_H
#define USER_PTHREAD_H

#include <type.h>

#define uint unsigned
#define PTHREAD_MUTEX_INITIALIZER \
    {{0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}, 0, 0, {0}, 0, 0, "test"}
#define PTHREAD_COND_INITIALIZER \
    {{0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}, 0, 0, {0}, "test"}
#define queue_size 20

/* detachstate-线程分离状态 */
#define PTHREAD_CREATE_DETACHED 0
#define PTHREAD_CREATE_JOINABLE 1

/* schedpolicy-线程调度策略 */
#define SCHED_FIFO 0
#define SCHED_RR 1
#define SCHED_OTHER 2

typedef struct spinlock {
    uint locked; // Is the lock held?

    // For debugging:
    char *name;   // Name of lock.
    int cpu;      // The number of the cpu holding the lock.
    uint pcs[10]; // The call stack (an array of program counters)
                  // that locked the lock.
} SPIN_LOCK;

/* schedparam-线程调度参数 */

/*
schedparam需要涉及到对timespec定义，
但是个人能力有限，没能读完全部内容
所以就仅仅建立一个结构体代表schedparam
*/

typedef struct p_sched_param {
    /* DATA */
} SCHED_PARAM;

/* inheritsched-线程的继承性 */
#define PTHREAD_INHERIT_SCHED 0
#define PTHREAD_EXPLICIT_SCHED 1

/* scope-线程作用域 */
#define PTHREAD_SCOPE_PROCESS 0
#define PTHREAD_SCOPE_SYSTEM 1

typedef struct p_pthread_attr_t {
    int detachstate;
    int schedpolicy;
    SCHED_PARAM schedparam;
    int inheritsched;
    int scope;
    u32 guardsize;
    int stackaddr_set;
    void *stackaddr;
    u32 stacksize;
} pthread_attr_t;

typedef struct {
    SPIN_LOCK lock; // 自旋锁:在获取锁之前一直处于忙等(自旋)阻塞状态 值为1
    uint nusers; // 记录当前有多少线程需要这个互斥锁
    uint owner; // 用来记录持有当前mutex的线程id，如果没有线程持有，这个值为0
    // 等待该互斥变量的线程队列
    int queue_wait[queue_size];
    int head;
    int tail;
    // For debugging:
    char *name; // 锁的名称
} pthread_mutex_t;

typedef struct {
    char *name;
    // char size[256];
    //   long int align;
} pthread_mutexattr_t;

typedef struct {
    SPIN_LOCK lock;
    int head; // 等待队列头部
    int tail; // 等待队列尾部
    int queue[queue_size];
    // For debugging:
    char *name;
} pthread_cond_t;

typedef struct {
    char *name;
} pthread_condattr_t;

int pthread_mutex_init(
    pthread_mutex_t *mutex,
    pthread_mutexattr_t *mutexattr); // added by ZengHao & MaLinhan 2021.12.23
int pthread_mutex_destroy(
    pthread_mutex_t *mutex); // added by ZengHao & MaLinhan 2021.12.23
int pthread_mutex_lock(
    pthread_mutex_t *mutex); // added by ZengHao & MaLinhan 2021.12.23
int pthread_mutex_unlock(
    pthread_mutex_t *mutex); // added by ZengHao & MaLinhan 2021.12.23
int pthread_mutex_trylock(
    pthread_mutex_t *mutex); // added by ZengHao & MaLinhan 2021.12.23
int pthread_cond_init(pthread_cond_t *cond,
                      const pthread_condattr_t
                          *cond_attr); // added by ZengHao & MaLinhan 2021.12.23
int pthread_cond_wait(
    pthread_cond_t *cond,
    pthread_mutex_t *mutex); // added by ZengHao & MaLinhan 2021.12.23
int pthread_cond_timewait(
    pthread_cond_t *cond, pthread_mutex_t *mutex,
    int *timeout); // added by ZengHao & MaLinhan 2021.12.23
int pthread_cond_signal(
    pthread_cond_t *cond); // added by ZengHao & MaLinhan 2021.12.23
int pthread_cond_broadcast(
    pthread_cond_t *cond); // added by ZengHao & MaLinhan 2021.12.23
int pthread_cond_destroy(
    pthread_cond_t *cond); // added by ZengHao & MaLinhan 2021.12.23

#endif
