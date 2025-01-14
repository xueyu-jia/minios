#include <minios/semaphore.h>
#include <minios/sched.h>

semaphore_t proc_table_sem;

int ksem_init(semaphore_t *sem, int max) {
    sem->value = max;
    sem->maxValue = max;
    sem->active = 1;
    spinlock_init(&sem->lock, "semaphore");
    return 0;
}

int ksem_destroy(semaphore_t *sem) {
    spinlock_acquire(&(sem->lock));
    // check if the entry is actived
    if ((*sem).active != 1) {
        spinlock_release(&(sem->lock));
        return -1;
    }
    (*sem).active = 0;
    spinlock_release(&(sem->lock));

    return 0;
}

int ksem_wait(semaphore_t *sem, int count) {
    spinlock_acquire(&(sem->lock));
    // check if the entry is actived
    if ((*sem).active == 0) {
        spinlock_release(&(sem->lock));
        return -1;
    }

    // if((*sem).maxValue <count)
    // {
    //   spinlock_release(&(sem->lock));
    //   return -1;
    // }
    // deleted by zhenhao 2023.5.20

    // if there is no enough lock, sleep
    while (((*sem).value) <= count - 1) { wait_for_sem(sem, &(sem->lock)); }
    // the thread enter the critical section, grap a lock
    (*sem).value -= count;
    spinlock_release(&(sem->lock));

    return 0;
}

int ksem_trywait(semaphore_t *sem, int count) {
    spinlock_acquire(&(sem->lock));
    // check if the entry is actived
    if ((*sem).active == 0) {
        spinlock_release(&(sem->lock));
        return -1;
    }
    // check if count < maxValue
    if ((*sem).maxValue < count) {
        spinlock_release(&(sem->lock));
        return -1;
    }
    // if there is no enough lock, exit
    if ((*sem).value <= count - 1) {
        spinlock_release(&(sem->lock));
        return -1;
    }
    // the thread enter the critical section, grap a lock
    (*sem).value -= count;
    spinlock_release(&(sem->lock));

    return 0;
}

int ksem_post(semaphore_t *sem, int count) {
    spinlock_acquire(&(sem->lock));
    // check if the entry is actived
    if ((*sem).active == 0) {
        spinlock_release(&(sem->lock));
        return -1;
    }
    // the thread exit the critical section, return a lock
    (*sem).value += count;
    // if there is any available lock, call wakeup
    if ((*sem).value > 0) wakeup_for_sem(sem);
    spinlock_release(&(sem->lock));

    return 0;
}

int ksem_getvalue(semaphore_t *sem) {
    spinlock_acquire(&(sem->lock));
    // check if the entry is actived
    if ((*sem).active == 0) {
        spinlock_release(&(sem->lock));
        return -1;
    }
    int value1;
    value1 = sem->value;
    spinlock_release(&(sem->lock));
    return value1;
}
