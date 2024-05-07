/**********************************************************
*	spinlock.c       //added by mingxuan 2018-12-26
***********************************************************/
// Mutual exclusion spin locks.

#include "spinlock.h"
#include "proc.h"
#define lock_log(info) { disp_str(info); disp_int(p_proc_current->task.pid); disp_str(" ");}
//extern int use_console_lock;

static inline uint
cmpxchg(uint oldval, uint newval, volatile uint* lock_addr)
{
  uint result;
  asm volatile("lock\n cmpxchg %0, %2" :
               "+m" (*lock_addr), "=a" (result) :
               "r"(newval), "1"(oldval) :
               "cc");
  return result;
}

void
initlock(struct spinlock *lock, char *name)
{
  lock->name = name;
  lock->locked = 0;
  lock->cpu = 0xffffffff;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// (Because contention is handled by spinning, must not
// go to sleep holding any locks.)
void
acquire(struct spinlock *lock)
{
  
  while(cmpxchg(0, 1, &lock->locked) == 1);
  lock->pcs[0] = proc2pid(p_proc_current);
}

// Release the lock.
void release(struct spinlock *lock)
{
  lock->locked = 0;
}

// 尝试对lock上锁，如果是锁着的状态则调用callback
void lock_or(struct spinlock *lock, void (*callback)()) {
    while (cmpxchg(0, 1, &lock->locked) == 1) {
        if (callback != NULL) { callback(); }
    }
    lock->pcs[0] = proc2pid(p_proc_current);
}

// 尝试对lock上锁，如果是锁着的状态则放弃cpu进行调度
void lock_or_yield(struct spinlock *lock) {
  return lock_or(lock, sched_yield); 
}



