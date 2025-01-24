#pragma once

#include <minios/spinlock.h>
#include <klib/stdint.h>

typedef int pthread_t;
typedef void* (*pthread_entry_t)(void*);

/* detachstate-线程分离状态 */
#define PTHREAD_CREATE_DETACHED 0
#define PTHREAD_CREATE_JOINABLE 1

/* schedpolicy-线程调度策略 */
#define SCHED_FIFO 0
#define SCHED_RR 1
#define SCHED_OTHER 2

#define QUEUE_SIZE 20

/* schedparam-线程调度参数 */

/*
schedparam需要涉及到对timespec定义，
但是个人能力有限，没能读完全部内容
所以就仅仅建立一个结构体代表schedparam
*/

typedef struct sched_param {
    int sched_priority;
} sched_param_t;

/* inheritsched-线程的继承性 */
#define PTHREAD_INHERIT_SCHED 0
#define PTHREAD_EXPLICIT_SCHED 1

/* scope-线程作用域 */
#define PTHREAD_SCOPE_PROCESS 0
#define PTHREAD_SCOPE_SYSTEM 1

typedef struct p_pthread_attr_t {
    int detachstate;
    int schedpolicy;
    sched_param_t schedparam;
    int inheritsched;
    int scope;
    u32 guardsize;
    int stackaddr_set;
    void* stackaddr;
    u32 stacksize;
} pthread_attr_t;

typedef struct {
    spinlock_t lock;
    uint nusers;
    uint owner;
    int queue_wait[QUEUE_SIZE];
    int head;
    int tail;
    char* name;
} pthread_mutex_t;

typedef struct {
    char* name;
} pthread_mutexattr_t;

typedef struct {
    spinlock_t lock;
    int head;
    int tail;
    int queue[QUEUE_SIZE];
    char* name;
} pthread_cond_t;

typedef struct {
    char* name;
} pthread_condattr_t;

pthread_t kern_pthread_self();
int kern_pthread_create_internal(pthread_t* thread, const pthread_attr_t* attr,
                                 pthread_entry_t wrapped_start_routine, void* arg);
int kern_pthread_join(pthread_t thread, void** retval);
void kern_pthread_exit(void* retval);
int kern_pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr);
int kern_pthread_cond_destroy(pthread_cond_t* cond);
int kern_pthread_cond_signal(pthread_cond_t* cond);
int kern_pthread_cond_broadcast(pthread_cond_t* cond);
int kern_pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int kern_pthread_cond_timewait(pthread_cond_t* cond, pthread_mutex_t* mutex, int* timeout);
int kern_pthread_mutex_init(pthread_mutex_t* mutex, pthread_mutexattr_t* attr);
int kern_pthread_mutex_destroy(pthread_mutex_t* mutex);
int kern_pthread_mutex_lock(pthread_mutex_t* mutex);
int kern_pthread_mutex_trylock(pthread_mutex_t* mutex);
int kern_pthread_mutex_unlock(pthread_mutex_t* mutex);
