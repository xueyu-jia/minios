/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-14 13:24:41
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-19 20:53:05
 * @FilePath: /minios/os/include/klib/spinlock.h
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
/**********************************************************
*	spinlock.h       //added by mingxuan 2018-12-26
***********************************************************/
#pragma once
#include <klib/list.h> // dbg
// Mutual exclusion lock.
#define uint unsigned

#define uint unsigned
#define PTHREAD_MUTEX_INITIALIZER {{0,0,0,{0}},0,0,{0},0,0,"test"}
#define PTHREAD_COND_INITIALIZER {{0,0,0,{0}},0,0,{0},"test"}
#define queue_size 20

typedef struct spinlock {
  uint locked;   // Is the lock held?

  // For debugging:
  char *name;    // Name of lock.
  int  cpu;      // The number of the cpu holding the lock.
  uint pcs[5];  // The call stack (an array of program counters)
                 // that locked the lock.
} SPIN_LOCK;


typedef struct{
  SPIN_LOCK lock;
	int head;//等待队列头部
	int tail;//等待队列尾部
	int queue[queue_size];
	// For debugging:
	char *name;
}pthread_cond_t;

typedef struct{
	char *name;
}pthread_condattr_t;


void initlock(struct spinlock *lock, char *name);
// Acquire the lock.
// Loops (spins) until the lock is acquired.
// (Because contention is handled by spinning, must not
// go to sleep holding any locks.)
void acquire(struct spinlock *lock);
// Release the lock.
void release(struct spinlock *lock);
// 尝试对lock上锁，如果是锁着的状态则调用callback
void lock_or(struct spinlock *lock, void (*callback)());
// 尝试对lock上锁，如果是锁着的状态则放弃cpu进行调度
void lock_or_yield(struct spinlock *lock);
