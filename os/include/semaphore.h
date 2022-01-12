/*
   By cjj and jjx    2021-12-22
   信号量实现的头文件。有函数声明与信号量结构定义。
*/

#ifndef SEMA_H
#define SEMA_H

#include "proto.h"
#include "spinlock.h"

/*信号量*/
struct Semaphore
{
   int value;
   int active;
   struct spinlock lock;
};

int ksem_init(struct Semaphore *sem, int max);
int ksem_destroy(struct Semaphore *sem);
int ksem_wait(struct Semaphore *sem, int count);
int ksem_post(struct Semaphore *sem, int count);
int ksem_trywait(struct Semaphore *sem,int count);
int ksem_getvalue(struct Semaphore *sem);


#endif  /* SEMAPHORE_H */