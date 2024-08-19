/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-14 13:24:41
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-19 10:44:36
 * @FilePath: /minios/os/klib/spinlock.c
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
/**********************************************************
*	spinlock.c       //added by mingxuan 2018-12-26
***********************************************************/
// Mutual exclusion spin locks.

#include <klib/spinlock.h>
#include <kernel/proc.h>
#include <klib/cmpxchg.h>

#define lock_log(info) { disp_str(info); disp_int(p_proc_current->task.pid); disp_str(" ");}
//extern int use_console_lock;

// static inline uint
// cmpxchg(uint oldval, uint newval, volatile uint* lock_addr)
// {

// 	// uint result;
// 	// asm volatile("lock\n cmpxchg %0, %2" :
// 	// 			"+m" (*lock_addr), "=a" (result) :
// 	// 			"r"(newval), "1"(oldval) :
// 	// 			"cc");
// 	// return result;
// 	__cmpxchg(ptr, old, new, size);
// }

static inline u32
cmpxchg(u32* ptr, u32 old, u32 new)
{
	return (u32)__cmpxchg(ptr, old, new, __X86_CASE_L);
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

	while(cmpxchg(&lock->locked, 0, 1) == 1);
	lock->pcs[0] = proc2pid(p_proc_current);
}

// Release the lock.
void release(struct spinlock *lock)
{
	lock->locked = 0;
}

// 尝试对lock上锁，如果是锁着的状态则调用callback
void lock_or(struct spinlock *lock, void (*callback)()) {
    while (cmpxchg(&lock->locked, 0, 1) == 1) {
        if (callback != NULL) { callback(); }
    }
    lock->pcs[0] = proc2pid(p_proc_current);
}

// 尝试对lock上锁，如果是锁着的状态则放弃cpu进行调度
void lock_or_yield(struct spinlock *lock) {
	return lock_or(lock, sched_yield);
}
