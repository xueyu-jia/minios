#include <pthread.h>
#include <sys/syscall.h>
#include <assert.h>

pthread_t pthread_self() {
    return syscall(NR_pthread_self);
}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, pthread_entry_t start_routine,
                   void* arg) {
    return syscall(NR_pthread_create, thread, attr, start_routine, arg);
}

int pthread_join(pthread_t thread, void** retval) {
    return syscall(NR_pthread_join, thread, retval);
}

void pthread_exit(void* retval) {
    syscall(NR_pthread_exit, retval);
    unreachable();
}

int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr) {
    return syscall(NR_pthread_cond_init, cond, attr);
}

int pthread_cond_destroy(pthread_cond_t* cond) {
    return syscall(NR_pthread_cond_destroy, cond);
}

int pthread_cond_signal(pthread_cond_t* cond) {
    return syscall(NR_pthread_cond_signal, cond);
}

int pthread_cond_broadcast(pthread_cond_t* cond) {
    return syscall(NR_pthread_cond_broadcast, cond);
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    return syscall(NR_pthread_cond_wait, cond, mutex);
}

int pthread_cond_timewait(pthread_cond_t* cond, pthread_mutex_t* mutex, int* timeout) {
    return syscall(NR_pthread_cond_timewait, cond, mutex, timeout);
}

int pthread_mutex_init(pthread_mutex_t* mutex, pthread_mutexattr_t* attr) {
    return syscall(NR_pthread_mutex_init, mutex, attr);
}

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
    return syscall(NR_pthread_mutex_destroy, mutex);
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
    return syscall(NR_pthread_mutex_lock, mutex);
}

int pthread_mutex_trylock(pthread_mutex_t* mutex) {
    return syscall(NR_pthread_mutex_trylock, mutex);
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    return syscall(NR_pthread_mutex_unlock, mutex);
}
