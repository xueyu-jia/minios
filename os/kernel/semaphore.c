/*
*   By cjj and jjx    12.22
*   信号量实现。有函数定义。
*/


#include "../include/type.h"
#include "../include/const.h"
#include "../include/protect.h"
#include "../include/string.h"
#include "../include/proc.h"
#include "../include/global.h"
#include "../include/proto.h"
#include "../include/fs_const.h"
#include "../include/hd.h"
#include "../include/fs.h"
#include "../include/buddy.h"
#include "../include/semaphore.h"

struct 	Semaphore	proc_table_sem;


int ksem_init(struct Semaphore *sem, int max)
{
  sem->value=max;
  sem->maxValue = max;
  sem->active=1;
  initlock(&sem->lock,"semaphore");
  return 0;

}


int ksem_destroy(struct Semaphore *sem)
{


  acquire(&(sem->lock));
  // check if the entry is actived
  if((*sem).active != 1) 
  {
    release(&(sem->lock));
    return -1;
  }
  (*sem).active = 0;
  release(&(sem->lock));

  return 0;
}

int ksem_wait(struct Semaphore *sem, int count)
{
  
  acquire(&(sem->lock));
  // check if the entry is actived
  if((*sem).active == 0) 
  {
    release(&(sem->lock));
    return -1;
  }

  // if((*sem).maxValue <count)
  // {
  //   release(&(sem->lock));
  //   return -1;
  // }
  // deleted by zhenhao 2023.5.20

  // if there is no enough lock, sleep
  while(((*sem).value) <= count - 1)
  {
    wait_for_sem(sem, &(sem->lock));
  }
  //the thread enter the critical section, grap a lock
  (*sem).value -= count;
  release(&(sem->lock));
  
  return 0;
}

int ksem_trywait(struct Semaphore *sem,int count)
{
  acquire(&(sem->lock));
  // check if the entry is actived
  if((*sem).active == 0) 
  {
    release(&(sem->lock));
    return -1;
  }
  // check if count < maxValue
  if((*sem).maxValue <count)
  {
    release(&(sem->lock));
    return -1;
  }
  // if there is no enough lock, exit
  if((*sem).value <= count - 1)
  {
    release(&(sem->lock));
    return -1;
  }
  //the thread enter the critical section, grap a lock
  (*sem).value -= count;
  release(&(sem->lock));

  return 0;
}

int ksem_post(struct Semaphore *sem, int count)
{
  
  acquire(&(sem->lock));
  // check if the entry is actived
  if((*sem).active == 0) 
  {
    release(&(sem->lock));
    return -1;
  }
  // the thread exit the critical section, return a lock
  (*sem).value += count;
  // if there is any available lock, call wakeup
  if((*sem).value > 0) 
    wakeup_for_sem(sem);
  release(&(sem->lock));
  
  return 0;
}

int ksem_getvalue(struct Semaphore *sem)
{
  acquire(&(sem->lock));
  // check if the entry is actived
  if((*sem).active == 0) 
  {
    release(&(sem->lock));
    return -1;
  }
  int value1;
  value1 = sem->value;
  release(&(sem->lock));
  return value1;
}



