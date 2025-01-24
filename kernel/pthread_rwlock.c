#include <minios/pthread.h>
#include <minios/proc.h>
#include <minios/sched.h>
#include <atomic.h>
#include <stdbool.h>
#include <errno.h>

int kern_pthread_rwlock_init(pthread_rwlock_t* rwlock, const pthread_rwlockattr_t* attr) {
    UNUSED(attr);
    rwlock->atomic = 0;
    return 0;
}

int kern_pthread_rwlock_destroy(pthread_rwlock_t* rwlock) {
    UNUSED(rwlock);
    return 0;
}

int kern_pthread_rwlock_rdlock(pthread_rwlock_t* rwlock) {
    while (kern_pthread_rwlock_tryrdlock(rwlock) != 0) { sched_yield(); }
    return 0;
}

int kern_pthread_rwlock_tryrdlock(pthread_rwlock_t* rwlock) {
    const int old = compare_exchange_strong(&rwlock->atomic, 0, 1);
    if (old == 0) { return 0; }
    if (old < 0) { return -EBUSY; }
    const bool locked = compare_exchange_strong(&rwlock->atomic, old, old + 1) == old;
    return locked ? 0 : -EBUSY;
}

int kern_pthread_rwlock_wrlock(pthread_rwlock_t* rwlock) {
    while (kern_pthread_rwlock_trywrlock(rwlock) != 0) { sched_yield(); }
    return 0;
}

int kern_pthread_rwlock_trywrlock(pthread_rwlock_t* rwlock) {
    const bool locked = compare_exchange_strong(&rwlock->atomic, 0, -1) == 0;
    return locked ? 0 : -EBUSY;
}

int kern_pthread_rwlock_unlock(pthread_rwlock_t* rwlock) {
    if (compare_exchange_strong(&rwlock->atomic, 0, 0) <= 0) {
        compare_exchange_strong(&rwlock->atomic, -1, 0);
    } else {
        fetch_sub(&rwlock->atomic, 1);
    }
    return 0;
}
