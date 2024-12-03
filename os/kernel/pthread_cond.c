#include <kernel/clock.h>
#include <kernel/proc.h>
#include <kernel/proto.h>

/*						added by ZengHao & MaLinhan
 * 2021.12.23
 */

int suspend_with_cancellation(PROCESS* self,
                              int timeout) {  // 挂起等待条件满足后被唤醒
  int ticks0 = ticks;
  // p_proc_current->task.channel = &ticks;
  while (ticks - ticks0 < timeout) {
    if (self->task.suspended == READY)  //统一PCB state 20240314
    {
      self->task.stat = READY;
      rq_insert(self);
      return 0;
    }
    // self->task.stat = SLEEPING;
    // sched();
    wait_event(&ticks);  // 使用统一的上层函数而非直接修改底层PCB
  }
  // self->task.stat = READY;
  return -1;  //超时
}

int kern_pthread_cond_init(pthread_cond_t* cond,
                           const pthread_condattr_t* cond_attr) {
  cond->lock.locked = 0;  //自旋锁状态置0
  cond->head = 0;
  cond->tail = 0;  //等待队列初始化
  if (cond_attr != NULL) {
    cond->name = cond_attr->name;
  }
  return 0;
}

int do_pthread_cond_init(pthread_cond_t* cond,
                         const pthread_condattr_t* cond_attr) {
  return kern_pthread_cond_init(cond, cond_attr);
}

int sys_pthread_cond_init() {
  return do_pthread_cond_init((pthread_cond_t*)get_arg(1),
                              (pthread_condattr_t*)get_arg(2));
}

int kern_pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
  //条件量 加锁操作
  // acquire(&cond->mutex);
  acquire(&cond->lock);

  //条件量 插入条件的等待队列
  if ((cond->tail + 1) % queue_size == cond->head) {
    release(&cond->lock);
    return -1;  //队列满了
  }
  cond->queue[cond->tail++] = mutex->owner;
  cond->tail %= queue_size;

  p_proc_current->task.stat = SLEEPING;       //该线程挂起,等待唤醒
  p_proc_current->task.suspended = SLEEPING;  //该线程挂起,等待唤醒

  //条件量 操作完释放锁
  // release(&cond->mutex);
  release(&cond->lock);

  // 释放互斥变量，否则别的线程无法操作资源，导致条件一直无法满足
  kern_pthread_mutex_unlock(mutex);
  // 挂起等待条件满足后被唤醒
  int sign_timeout = suspend_with_cancellation(p_proc_current, MAX_INT);
  //线程唤醒之后就需要重新持有原来的锁
  kern_pthread_mutex_lock(mutex);

  // 取消点，等待期间被取消了
  if (p_proc_current->task.stat != SLEEPING) {
    //条件量 加锁操作
    // acquire(&cond->mutex);
    acquire(&cond->lock);
    int sign = 0;
    for (int i = cond->head; i != cond->tail; i = (i + 1) % queue_size) {
      if (cond->queue[i] == kern_pthread_self()) {
        for (int j = i; j != cond->tail; j = (j + 1) % queue_size) {
          cond->queue[j] = cond->queue[(j + 1) % queue_size];
          sign = 1;
          break;
        }
      }
    }
    if (sign) {
      cond->tail = (cond->tail - 1 + queue_size) % queue_size;
    }
    //条件量 操作完释放锁
    // release(&cond->mutex);
    release(&cond->lock);
  }
  return sign_timeout;
}

int do_pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
  return kern_pthread_cond_wait(cond, mutex);
}
int sys_pthread_cond_wait() {
  return do_pthread_cond_wait((pthread_cond_t*)get_arg(1),
                              (pthread_mutex_t*)get_arg(2));
}

int kern_pthread_cond_timewait(pthread_cond_t* cond, pthread_mutex_t* mutex,
                               int* timeout) {
  //条件量 加锁操作
  // acquire(&cond->mutex);
  acquire(&cond->lock);

  //条件量 插入条件的等待队列
  if ((cond->tail + 1) % queue_size == cond->head) {
    release(&cond->lock);
    return -1;  //队列满了
  }
  cond->queue[cond->tail++] = mutex->owner;
  cond->tail %= queue_size;

  p_proc_current->task.stat = SLEEPING;       //该线程挂起,等待唤醒
  p_proc_current->task.suspended = SLEEPING;  //该线程挂起,等待唤醒

  //条件量 操作完释放锁
  // release(&cond->mutex);
  release(&cond->lock);

  // 释放互斥变量，否则别的线程无法操作资源，导致条件一直无法满足
  kern_pthread_mutex_unlock(mutex);
  // 挂起等待条件满足后被唤醒
  int sign_timeout = suspend_with_cancellation(p_proc_current, *timeout);
  //线程唤醒之后就需要重新持有原来的锁
  kern_pthread_mutex_lock(mutex);

  // 取消点，等待期间被取消了
  if (p_proc_current->task.stat != SLEEPING) {
    //条件量 加锁操作
    // acquire(&cond->mutex);
    acquire(&cond->lock);
    int sign = 0;
    for (int i = cond->head; i != cond->tail; i = (i + 1) % queue_size) {
      if (cond->queue[i] == kern_pthread_self()) {
        for (int j = i; j != cond->tail; j = (j + 1) % queue_size) {
          cond->queue[j] = cond->queue[(j + 1) % queue_size];
          sign = 1;
          break;
        }
      }
    }
    if (sign) {
      cond->tail = (cond->tail - 1 + queue_size) % queue_size;
    }
    //条件量 操作完释放锁
    // release(&cond->mutex);
    release(&cond->lock);
  }
  return sign_timeout;
}

int do_pthread_cond_timewait(pthread_cond_t* cond, pthread_mutex_t* mutex,
                             int* timeout) {
  return kern_pthread_cond_timewait(cond, mutex, timeout);
}

int sys_pthread_cond_timewait() {
  return do_pthread_cond_timewait((pthread_cond_t*)get_arg(1),
                                  (pthread_mutex_t*)get_arg(2),
                                  (int*)get_arg(3));
}

int kern_pthread_cond_signal(pthread_cond_t* cond) {
  //条件量 加锁操作
  acquire(&cond->lock);

  // 取出一个被被阻塞的线程:默认排列顺序取出
  if (cond->head == cond->tail) {  //队列为空，唤醒线程失败,返回-1
    release(&cond->lock);
    return -1;
  }
  int th = cond->queue[cond->head++];
  cond->head %= queue_size;

  for (int i = 0; i < NR_PCBS; i++) {
    if (proc_table[i].task.pthread_id == th) {
      proc_table[i].task.suspended = READY;  //统一PCB state 20240314
    }
  }
  //条件量 操作完释放锁
  release(&cond->lock);
  return 0;
}

int do_pthread_cond_signal(pthread_cond_t* cond) {
  return kern_pthread_cond_signal(cond);
}
int sys_pthread_cond_signal() {
  return do_pthread_cond_signal((pthread_cond_t*)get_arg(1));
}

int kern_pthread_cond_broadcast(pthread_cond_t* cond) {
  //条件量 加锁操作
  acquire(&cond->lock);

  // 取出一个被被阻塞的线程:默认排列顺序取出
  for (int i = cond->head; i != cond->tail; i = (i + 1) % queue_size) {
    for (int j = 0; j < NR_PCBS; j++) {
      if (proc_table[j].task.pthread_id == cond->queue[i]) {
        proc_table[j].task.suspended = READY;  //统一PCB state 20240314
      }
    }
  }
  cond->head = 0;
  cond->tail = 0;
  //条件量 操作完释放锁
  release(&cond->lock);
  return 0;
}

int do_pthread_cond_broadcast(pthread_cond_t* cond) {
  return kern_pthread_cond_broadcast(cond);
}

int sys_pthread_cond_broadcast() {
  return do_pthread_cond_broadcast((pthread_cond_t*)get_arg(1));
}

int kern_pthread_cond_destroy(pthread_cond_t* cond) {
  acquire(&cond->lock);
  int num = (cond->tail - cond->head + queue_size) % queue_size;
  release(&cond->lock);
  if (num != 0) {  // 还有线程被挂起
    return -1;
  }
  cond->head = 0;
  cond->tail = 0;
  cond->lock.locked = 0;
  // kern_free_4k(cond);      // deleted by zhenhao 2023.3.5
  return 0;
}

int do_pthread_cond_destroy(pthread_cond_t* cond) {
  return kern_pthread_cond_destroy(cond);
}

int sys_pthread_cond_destroy() {
  return do_pthread_cond_destroy((pthread_cond_t*)get_arg(1));
}
