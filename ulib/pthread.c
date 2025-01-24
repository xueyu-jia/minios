#include <pthread.h>
#include <sys/syscall.h>
#include <malloc.h>
#include <assert.h>

typedef struct {
    pthread_entry_t start_routine;
    void* arg;
} wrapped_arg_t;

static void* pthread_routine_wrapper(void* wrapped_arg) {
    wrapped_arg_t* d = wrapped_arg;
    pthread_entry_t start_routine = d->start_routine;
    void* arg = d->arg;
    //! NOTE: here wrapped arg is allocated in caller th and free in callee th
    free(wrapped_arg);
    pthread_exit(start_routine(arg));
    unreachable();
}

pthread_t pthread_self() {
    return syscall(NR_pthread_self);
}

int pthread_create(pthread_t* thread, const pthread_attr_t* attr, pthread_entry_t start_routine,
                   void* arg) {
    wrapped_arg_t* d = malloc(sizeof(wrapped_arg_t));
    d->start_routine = start_routine;
    d->arg = arg;
    return syscall(NR_pthread_create, thread, attr, pthread_routine_wrapper, d);
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
