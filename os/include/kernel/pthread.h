#ifndef PTHREAD_H
#define PTHREAD_H

#include <kernel/type.h>
#include <klib/spinlock.h>

/* detachstate-线程分离状态 */
#define PTHREAD_CREATE_DETACHED 0
#define PTHREAD_CREATE_JOINABLE 1

/* schedpolicy-线程调度策略 */
#define SCHED_FIFO 0
#define SCHED_RR 1
#define SCHED_OTHER 2

/* schedparam-线程调度参数 */

/*
schedparam需要涉及到对timespec定义，
但是个人能力有限，没能读完全部内容
所以就仅仅建立一个结构体代表schedparam
*/

typedef struct p_sched_param {
  /* DATA */
  int sched_priority;  // added by dongzhangqi 2023.5.8
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
  void* stackaddr;
  u32 stacksize;
} pthread_attr_t;

typedef struct {
  SPIN_LOCK lock;  //自旋锁:在获取锁之前一直处于忙等(自旋)阻塞状态 值为1
  uint nusers;  //记录当前有多少线程需要这个互斥锁
  uint owner;  //用来记录持有当前mutex的线程id，如果没有线程持有，这个值为0
  //等待该互斥变量的线程队列
  int queue_wait[queue_size];
  int head;
  int tail;
  // For debugging:
  char* name;  // 锁的名称
} pthread_mutex_t;

typedef struct {
  char* name;
  // char size[256];
  //   long int align;
} pthread_mutexattr_t;

PUBLIC int do_pthread_mutex_init(pthread_mutex_t* mutex,
                                 pthread_mutexattr_t* mutexattr);
int sys_pthread_mutex_lock();
int do_pthread_mutex_lock(pthread_mutex_t* mutex);
PUBLIC pthread_t kern_pthread_self();
int do_pthread_mutex_trylock(pthread_mutex_t* mutex);
int sys_pthread_mutex_trylock();
int sys_pthread_mutex_unlock();
int do_pthread_mutex_unlock(pthread_mutex_t* mutex);
int sys_pthread_mutex_destroy();
int do_pthread_mutex_destroy(pthread_mutex_t* mutex);
int kern_pthread_mutex_unlock(pthread_mutex_t* mutex);
int kern_pthread_mutex_lock(pthread_mutex_t* mutex);
#endif
