#include <minios/pthread.h>
#include <minios/sched.h>
#include <minios/interrupt.h>
#include <minios/clock.h>
#include <minios/proc.h>
#include <limits.h>

static int suspend_with_cancellation(process_t* self, int timeout) {
    const int ticks0 = ticks;
    while (ticks - ticks0 < timeout) {
        if (self->task.suspended == READY) {
            self->task.stat = READY;
            disable_int_begin();
            rq_insert(self);
            disable_int_end();
            return 0;
        }
        wait_event(&ticks);
    }
    return -1; //<! timeout
}

int kern_pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* cond_attr) {
    cond->lock.locked = 0;
    cond->head = 0;
    cond->tail = 0; // 等待队列初始化
    if (cond_attr != NULL) { cond->name = cond_attr->name; }
    return 0;
}

int kern_pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    spinlock_acquire(&cond->lock);

    // 条件量 插入条件的等待队列
    if ((cond->tail + 1) % QUEUE_SIZE == cond->head) {
        spinlock_release(&cond->lock);
        return -1; // 队列满了
    }
    cond->queue[cond->tail++] = mutex->owner;
    cond->tail %= QUEUE_SIZE;

    p_proc_current->task.stat = SLEEPING;      // 该线程挂起,等待唤醒
    p_proc_current->task.suspended = SLEEPING; // 该线程挂起,等待唤醒

    // 条件量 操作完释放锁
    //  spinlock_release(&cond->mutex);
    spinlock_release(&cond->lock);

    kern_pthread_mutex_unlock(mutex);
    int sign_timeout = suspend_with_cancellation(p_proc_current, MAX_INT);
    kern_pthread_mutex_lock(mutex);

    // 取消点，等待期间被取消了
    if (p_proc_current->task.stat != SLEEPING) {
        // 条件量 加锁操作
        //  spinlock_acquire(&cond->mutex);
        spinlock_acquire(&cond->lock);
        int sign = 0;
        for (int i = cond->head; i != cond->tail; i = (i + 1) % QUEUE_SIZE) {
            if (cond->queue[i] == kern_pthread_self()) {
                for (int j = i; j != cond->tail; j = (j + 1) % QUEUE_SIZE) {
                    cond->queue[j] = cond->queue[(j + 1) % QUEUE_SIZE];
                    sign = 1;
                    break;
                }
            }
        }
        if (sign) { cond->tail = (cond->tail - 1 + QUEUE_SIZE) % QUEUE_SIZE; }
        // 条件量 操作完释放锁
        //  spinlock_release(&cond->mutex);
        spinlock_release(&cond->lock);
    }
    return sign_timeout;
}

int kern_pthread_cond_timedwait(pthread_cond_t* cond, pthread_mutex_t* mutex, int* timeout) {
    // 条件量 加锁操作
    //  spinlock_acquire(&cond->mutex);
    spinlock_acquire(&cond->lock);

    // 条件量 插入条件的等待队列
    if ((cond->tail + 1) % QUEUE_SIZE == cond->head) {
        spinlock_release(&cond->lock);
        return -1; // 队列满了
    }
    cond->queue[cond->tail++] = mutex->owner;
    cond->tail %= QUEUE_SIZE;

    p_proc_current->task.stat = SLEEPING;      // 该线程挂起,等待唤醒
    p_proc_current->task.suspended = SLEEPING; // 该线程挂起,等待唤醒

    // 条件量 操作完释放锁
    //  spinlock_release(&cond->mutex);
    spinlock_release(&cond->lock);

    // 释放互斥变量，否则别的线程无法操作资源，导致条件一直无法满足
    kern_pthread_mutex_unlock(mutex);
    // 挂起等待条件满足后被唤醒
    int sign_timeout = suspend_with_cancellation(p_proc_current, *timeout);
    // 线程唤醒之后就需要重新持有原来的锁
    kern_pthread_mutex_lock(mutex);

    // 取消点，等待期间被取消了
    if (p_proc_current->task.stat != SLEEPING) {
        // 条件量 加锁操作
        //  spinlock_acquire(&cond->mutex);
        spinlock_acquire(&cond->lock);
        int sign = 0;
        for (int i = cond->head; i != cond->tail; i = (i + 1) % QUEUE_SIZE) {
            if (cond->queue[i] == kern_pthread_self()) {
                for (int j = i; j != cond->tail; j = (j + 1) % QUEUE_SIZE) {
                    cond->queue[j] = cond->queue[(j + 1) % QUEUE_SIZE];
                    sign = 1;
                    break;
                }
            }
        }
        if (sign) { cond->tail = (cond->tail - 1 + QUEUE_SIZE) % QUEUE_SIZE; }
        // 条件量 操作完释放锁
        //  spinlock_release(&cond->mutex);
        spinlock_release(&cond->lock);
    }
    return sign_timeout;
}

int kern_pthread_cond_signal(pthread_cond_t* cond) {
    // 条件量 加锁操作
    spinlock_acquire(&cond->lock);

    // 取出一个被被阻塞的线程:默认排列顺序取出
    if (cond->head == cond->tail) { // 队列为空，唤醒线程失败,返回-1
        spinlock_release(&cond->lock);
        return -1;
    }
    int th = cond->queue[cond->head++];
    cond->head %= QUEUE_SIZE;

    for (int i = 0; i < NR_PCBS; ++i) {
        if (proc_table[i].task.tid == th) {
            proc_table[i].task.suspended = READY; // 统一PCB state 20240314
        }
    }
    // 条件量 操作完释放锁
    spinlock_release(&cond->lock);
    return 0;
}

int kern_pthread_cond_broadcast(pthread_cond_t* cond) {
    // 条件量 加锁操作
    spinlock_acquire(&cond->lock);

    // 取出一个被被阻塞的线程:默认排列顺序取出
    for (int i = cond->head; i != cond->tail; i = (i + 1) % QUEUE_SIZE) {
        for (int j = 0; j < NR_PCBS; ++j) {
            if (proc_table[j].task.tid == cond->queue[i]) {
                proc_table[j].task.suspended = READY; // 统一PCB state 20240314
            }
        }
    }
    cond->head = 0;
    cond->tail = 0;
    // 条件量 操作完释放锁
    spinlock_release(&cond->lock);
    return 0;
}

int kern_pthread_cond_destroy(pthread_cond_t* cond) {
    spinlock_acquire(&cond->lock);
    int num = (cond->tail - cond->head + QUEUE_SIZE) % QUEUE_SIZE;
    spinlock_release(&cond->lock);
    // 还有线程被挂起
    if (num != 0) { return -1; }
    cond->head = 0;
    cond->tail = 0;
    cond->lock.locked = 0;
    return 0;
}
