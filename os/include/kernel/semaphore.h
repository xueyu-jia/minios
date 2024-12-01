/*
 * @Author: lirong lirongleiyang@163.com
 * @Date: 2024-08-14 13:24:41
 * @LastEditors: lirong lirongleiyang@163.com
 * @LastEditTime: 2024-08-19 17:37:34
 * @FilePath: /minios/os/include/kernel/semaphore.h
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
/*
   By cjj and jjx    2021-12-22
   信号量实现的头文件。有函数声明与信号量结构定义。
*/

#ifndef SEMA_H
#define SEMA_H

#include <kernel/proto.h>
#include <klib/spinlock.h>

/*信号量*/
struct Semaphore {
  int value;
  int maxValue;
  int active;
  struct spinlock lock;
};

int ksem_init(struct Semaphore *sem, int max);
int ksem_destroy(struct Semaphore *sem);
int ksem_wait(struct Semaphore *sem, int count);
int ksem_post(struct Semaphore *sem, int count);
int ksem_trywait(struct Semaphore *sem, int count);
int ksem_getvalue(struct Semaphore *sem);

extern struct Semaphore proc_table_sem;
#endif /* SEMAPHORE_H */
