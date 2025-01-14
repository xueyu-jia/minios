#pragma once

#include <klib/stdint.h>

typedef struct spinlock {
    uint locked; // Is the lock held?

    // For debugging:
    char *name;  // Name of lock.
    int cpu;     // The number of the cpu holding the lock.
    uint pcs[5]; // The call stack (an array of program counters)
                 // that locked the lock.
} spinlock_t;

void spinlock_init(spinlock_t *lock, char *name);
void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);
void spinlock_lock_or(spinlock_t *lock, void (*callback)());
void spinlock_lock_or_yield(spinlock_t *lock);
