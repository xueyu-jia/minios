#include <minios/spinlock.h>
#include <minios/proc.h>
#include <minios/sched.h>
#include <klib/cmpxchg.h>

static inline u32 cmpxchg(u32 *ptr, u32 old, u32 new) {
    return (u32)__cmpxchg(ptr, old, new, __X86_CASE_L);
}

void spinlock_init(spinlock_t *lock, char *name) {
    lock->name = name;
    lock->locked = 0;
    lock->cpu = 0xffffffff;
}

void spinlock_acquire(spinlock_t *lock) {
    while (cmpxchg(&lock->locked, 0, 1) == 1);
    lock->pcs[0] = proc2pid(p_proc_current);
}

void spinlock_release(spinlock_t *lock) {
    lock->locked = 0;
}

void spinlock_lock_or(spinlock_t *lock, void (*callback)()) {
    while (cmpxchg(&lock->locked, 0, 1) == 1) {
        if (callback != NULL) { callback(); }
    }
    lock->pcs[0] = proc2pid(p_proc_current);
}

void spinlock_lock_or_yield(spinlock_t *lock) {
    return spinlock_lock_or(lock, sched_yield);
}
