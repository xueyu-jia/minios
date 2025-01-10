#include <stdlib.h>
#include <atomic.h>
#include <stdbool.h>
#include <stddef.h>

int exchange(int *ptr, int value) {
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

int compare_exchange_strong(int *value, int expected, int desired) {
    int tmp = expected;
    __atomic_compare_exchange_n(value, &tmp, desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return tmp;
}

int compare_exchange_weak(int *value, int expected, int desired) {
    int tmp = expected;
    __atomic_compare_exchange_n(value, &tmp, desired, true, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return tmp;
}

void fetch_add(void *atomic, int value) {
    __sync_fetch_and_add((int *)atomic, value);
}

void fetch_sub(void *atomic, int value) {
    __sync_fetch_and_sub((int *)atomic, value);
}

void atomic_inc(void *atomic) {
    fetch_add(atomic, 1);
}

void atomic_dec(void *atomic) {
    fetch_sub(atomic, 1);
}

bool try_lock(void *lock) {
    return compare_exchange_strong((int *)lock, 0, 1) == 0;
}

void acquire(void *lock) {
    while (compare_exchange_strong((int *)lock, 0, 1) != 0) {}
}

void lock_or(void *lock, void (*callback)()) {
    while (compare_exchange_strong((int *)lock, 0, 1) != 0) {
        if (callback != NULL) { callback(); }
    }
}

void release(void *lock) {
    exchange((int *)lock, 0);
}
