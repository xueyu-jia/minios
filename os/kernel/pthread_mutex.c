#include <kernel/proc.h>
#include <kernel/proto.h>
#include <kernel/pthread.h>
#include <klib/spinlock.h>

/*						added by ZengHao & MaLinhan
 * 2021.12.23
 */

int mutex_suspend_with_cancellation(
    PROCESS* self, pthread_mutex_t* mutex) { // 挂起等待条件满足后被唤醒
    while (1) {
        if (self->task.suspended == READY) // 统一PCB state 20240314
        {
            self->task.stat = READY;
            mutex->owner = self->task.pthread_id;
            return 0;
        }
    }
}
// int mutex_suspend_with_cancellation(PROCESS *self, pthread_mutex_t *mutex)
// { // 挂起等待条件满足后被唤醒
//   while (self->task.stat == SLEEPING)
//       ;
//   mutex->owner = self->task.pthread_id;
//   return 0;
// }

PUBLIC int kern_pthread_mutex_init(pthread_mutex_t* mutex,
                                   pthread_mutexattr_t* mutexattr) {
    mutex->lock.locked = 0; // 自旋锁置为0
    mutex->nusers = 0;      // 获取锁的线程数量为0
    mutex->owner = 0;       // 当前持有该锁的线程不存在
    if (mutexattr != NULL) { mutex->name = mutexattr->name; }
    // 初始化等待队列
    mutex->head = 0;
    mutex->tail = 0;

    return 0;
}
PUBLIC int sys_pthread_mutex_init() {
    return do_pthread_mutex_init((pthread_mutex_t*)get_arg(1),
                                 (pthread_mutexattr_t*)get_arg(2));
}

PUBLIC int do_pthread_mutex_init(pthread_mutex_t* mutex,
                                 pthread_mutexattr_t* mutexattr) {
    return kern_pthread_mutex_init(mutex, mutexattr);
}

int kern_pthread_mutex_lock(pthread_mutex_t* mutex) {
    acquire(&mutex->lock);

    if (mutex->owner == 0) {
        // 所持有者为当前线程
        mutex->owner = kern_pthread_self();
        // 需要锁的线程+1
        mutex->nusers++;
        // release(mutex);
        release(&mutex->lock);
        return 0;
    }

    // 需要锁的线程+1
    mutex->nusers++;
    // 获取失败，需要阻塞，把当前线程插入该互斥锁的等待队列
    if ((mutex->tail + 1) % queue_size == mutex->head) {
        release(&mutex->lock);
        return -1; // 队列满了
    }
    mutex->queue_wait[mutex->tail++] = kern_pthread_self();
    mutex->tail %= queue_size;

    // 该线程挂起,等待唤醒
    p_proc_current->task.stat = SLEEPING;
    p_proc_current->task.suspended = SLEEPING;

    // release(mutex);
    release(&mutex->lock);

    // 挂起等待唤醒
    mutex_suspend_with_cancellation(p_proc_current, mutex);
    return 0;
}

int sys_pthread_mutex_lock() // 阻塞式获取互斥变量
{
    return do_pthread_mutex_lock((pthread_mutex_t*)get_arg(1));
}

int do_pthread_mutex_lock(pthread_mutex_t* mutex) {
    return kern_pthread_mutex_lock(mutex);
}

int kern_pthread_mutex_trylock(pthread_mutex_t* mutex) {
    acquire(&mutex->lock);
    if (mutex->nusers == 0) {
        // 所持有者为当前线程
        mutex->owner = kern_pthread_self();
        // 等待锁的线程+1
        mutex->nusers++;

        release(&mutex->lock);
        return 0; // 成功返回0
    }
    // release(mutex);
    release(&mutex->lock);
    return -1; // 失败返回-1
}

int sys_pthread_mutex_trylock() // 非阻塞式获取互斥变量
{
    return do_pthread_mutex_trylock((pthread_mutex_t*)get_arg(1));
}

int do_pthread_mutex_trylock(pthread_mutex_t* mutex) {
    return kern_pthread_mutex_trylock(mutex);
}

int kern_pthread_mutex_unlock(pthread_mutex_t* mutex) {
    acquire(&mutex->lock);
    // 当前锁被释放，不存在拥有者
    mutex->owner = 0;
    // 等待锁的线程-1
    mutex->nusers--;
    // 弹出队首元素
    if (mutex->nusers > 0) {
        int th = mutex->queue_wait[mutex->head++];
        mutex->head %= queue_size;

        // 唤醒队首元素对应的线程
        for (int i = 0; i < NR_PCBS; i++) {
            if (proc_table[i].task.pthread_id == th) {
                proc_table[i].task.suspended = READY; // 统一PCB state 20240314
            }
        }
    }

    release(&mutex->lock);
    return 0;
}
int sys_pthread_mutex_unlock() {
    return do_pthread_mutex_unlock((pthread_mutex_t*)get_arg(1));
}

int do_pthread_mutex_unlock(pthread_mutex_t* mutex) {
    return kern_pthread_mutex_unlock(mutex);
}

int kern_pthread_mutex_destroy(pthread_mutex_t* mutex) {
    acquire(&mutex->lock);

    // 期望持有该锁的线程数量
    int nurses = mutex->nusers;

    release(&mutex->lock);

    if (nurses != 0) { // 正在被使用
        return -1;
    }
    // kern_free_4k(mutex);   // deleted by zhenhao 2023.3.5
    return 0;
}
int sys_pthread_mutex_destroy() {
    return do_pthread_mutex_destroy((pthread_mutex_t*)get_arg(1));
}

int do_pthread_mutex_destroy(pthread_mutex_t* mutex) {
    return kern_pthread_mutex_destroy(mutex);
}

/*							end added
 */
