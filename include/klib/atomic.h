#pragma once

#include <atomic.h>
#include <compiler.h>
#include <klib/cmpxchg.h>

typedef struct {
    int counter;
} atomic_t;

static inline int atomic_dec_and_test(atomic_t *v) {
    unsigned char c;
    asm volatile("lock; decl %0; sete %1" : "+m"(v->counter), "=qm"(c) : : "memory");
    return c != 0;
}

#define atomic_set(v, i) (((v)->counter) = (i))
#define atomic_get(v) ((v)->counter)
