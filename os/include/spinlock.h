/**********************************************************
*	spinlock.h       //added by mingxuan 2018-12-26
***********************************************************/
#pragma once

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
  uint pcs[10];  // The call stack (an array of program counters)
                 // that locked the lock.
} SPIN_LOCK;

typedef struct{
  SPIN_LOCK lock;//自旋锁:在获取锁之前一直处于忙等(自旋)阻塞状态 值为1 
  uint nusers;//记录当前有多少线程需要这个互斥锁
  uint owner;//用来记录持有当前mutex的线程id，如果没有线程持有，这个值为0
  //等待该互斥变量的线程队列
  int queue_wait[queue_size];
  int head;
  int tail;
  // For debugging:
  char *name;    // 锁的名称
}pthread_mutex_t;

typedef struct{
  char *name;
  // char size[256];
//   long int align;
}pthread_mutexattr_t;


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
