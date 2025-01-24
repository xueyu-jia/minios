#pragma once

#include <stddef.h>
#include <compiler.h>

typedef struct {
    unsigned int locked;
    char resvered[28];
} pthread_spinlock_t;

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
    size_t guardsize;
    int stackaddr_set;
    void* stackaddr;
    size_t stacksize;
} pthread_attr_t;

typedef struct {
    pthread_spinlock_t lock;
    size_t nusers;
    size_t owner;
    int queue_wait[QUEUE_SIZE];
    int head;
    int tail;
    char* name;
} pthread_mutex_t;

#define PTHREAD_MUTEX_INITIALIZER \
    {                             \
        .lock = {},               \
        .nusers = 0,              \
        .owner = 0,               \
        .queue_wait = {},         \
        .head = 0,                \
        .tail = 0,                \
        .name = "pthread.mutex",  \
    }

typedef struct {
    char* name;
} pthread_mutexattr_t;

typedef struct {
    pthread_spinlock_t lock;
    int head;
    int tail;
    int queue[QUEUE_SIZE];
    char* name;
} pthread_cond_t;

#define PTHREAD_COND_INITIALIZER \
    {                            \
        .lock = {},              \
        .head = 0,               \
        .tail = 0,               \
        .queue = {},             \
        .name = "pthread.cond",  \
    }

typedef struct {
    char* name;
} pthread_condattr_t;

typedef struct {
    int atomic;
    char* name;
} pthread_rwlock_t;

#define PTHREAD_RWLOCK_INITIALIZER \
    {                              \
        .atomic = 0,               \
        .name = "pthread.rwlock",  \
    }

typedef struct {
} pthread_rwlockattr_t;

pthread_t pthread_self();
int pthread_create(pthread_t* thread, const pthread_attr_t* attr, pthread_entry_t start_routine,
                   void* arg);
int pthread_join(pthread_t thread, void** retval);
NORETURN void pthread_exit(void* retval);

int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr);
int pthread_cond_destroy(pthread_cond_t* cond);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_broadcast(pthread_cond_t* cond);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int pthread_cond_timewait(pthread_cond_t* cond, pthread_mutex_t* mutex, int* timeout);

int pthread_mutex_init(pthread_mutex_t* mutex, pthread_mutexattr_t* attr);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_mutex_unlock(pthread_mutex_t* mutex);

int pthread_rwlock_init(pthread_rwlock_t* rwlock, const pthread_rwlockattr_t* attr);
int pthread_rwlock_destroy(pthread_rwlock_t* rwlock);
int pthread_rwlock_rdlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t* rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t* rwlock);
