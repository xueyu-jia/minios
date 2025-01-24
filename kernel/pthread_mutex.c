#include <minios/pthread.h>
#include <minios/proc.h>
#include <minios/sched.h>
#include <minios/spinlock.h>
#include <stdbool.h>

// 挂起等待条件满足后被唤醒
int mutex_suspend_with_cancellation(process_t* self, pthread_mutex_t* mutex) {
    while (true) {
        if (self->task.suspended == READY) {
            self->task.stat = READY;
            rq_insert(self);
            mutex->owner = self->task.tid;
            return 0;
        }
    }
}

int kern_pthread_mutex_init(pthread_mutex_t* mutex, pthread_mutexattr_t* mutexattr) {
    mutex->lock.locked = 0; // 自旋锁置为0
    mutex->nusers = 0;      // 获取锁的线程数量为0
    mutex->owner = 0;       // 当前持有该锁的线程不存在
    if (mutexattr != NULL) { mutex->name = mutexattr->name; }
    // 初始化等待队列
    mutex->head = 0;
    mutex->tail = 0;

    return 0;
}

int kern_pthread_mutex_lock(pthread_mutex_t* mutex) {
    spinlock_acquire(&mutex->lock);

    if (mutex->owner == 0) {
        // 所持有者为当前线程
        mutex->owner = kern_pthread_self();
        // 需要锁的线程+1
        mutex->nusers++;
        spinlock_release(&mutex->lock);
        return 0;
    }

    // 需要锁的线程+1
    mutex->nusers++;
    // 获取失败，需要阻塞，把当前线程插入该互斥锁的等待队列
    if ((mutex->tail + 1) % QUEUE_SIZE == mutex->head) {
        spinlock_release(&mutex->lock);
        return -1; // 队列满了
    }
    mutex->queue_wait[mutex->tail++] = kern_pthread_self();
    mutex->tail %= QUEUE_SIZE;

    // 该线程挂起,等待唤醒
    p_proc_current->task.suspended = SLEEPING;
    p_proc_current->task.stat = SLEEPING;
    rq_remove(p_proc_current);

    // spinlock_release(mutex);
    spinlock_release(&mutex->lock);

    // 挂起等待唤醒
    mutex_suspend_with_cancellation(p_proc_current, mutex);
    return 0;
}

int kern_pthread_mutex_trylock(pthread_mutex_t* mutex) {
    spinlock_acquire(&mutex->lock);
    if (mutex->nusers == 0) {
        // 所持有者为当前线程
        mutex->owner = kern_pthread_self();
        // 等待锁的线程+1
        mutex->nusers++;

        spinlock_release(&mutex->lock);
        return 0; // 成功返回0
    }
    // spinlock_release(mutex);
    spinlock_release(&mutex->lock);
    return -1; // 失败返回-1
}

int kern_pthread_mutex_unlock(pthread_mutex_t* mutex) {
    spinlock_acquire(&mutex->lock);
    // 当前锁被释放，不存在拥有者
    mutex->owner = 0;
    // 等待锁的线程-1
    mutex->nusers--;
    // 弹出队首元素
    if (mutex->nusers > 0) {
        int th = mutex->queue_wait[mutex->head++];
        mutex->head %= QUEUE_SIZE;

        // 唤醒队首元素对应的线程
        for (int i = 0; i < NR_PCBS; ++i) {
            if (proc_table[i].task.tid == th) {
                proc_table[i].task.suspended = READY; // 统一PCB state 20240314
                //! FIXME: we'd better not insert a sleeping thread into the sched queue, though
                //! it's assumed to be ready soon later
                //! NOTE: for instance, use wakeup here, but there's still execution order issues to
                //! be solved
                rq_insert(&proc_table[i]);
            }
        }
    }

    spinlock_release(&mutex->lock);
    return 0;
}

int kern_pthread_mutex_destroy(pthread_mutex_t* mutex) {
    spinlock_acquire(&mutex->lock);

    // 期望持有该锁的线程数量
    int nurses = mutex->nusers;

    spinlock_release(&mutex->lock);

    // 正在被使用
    if (nurses != 0) { return -1; }
    // kern_free_4k(mutex);
    return 0;
}
