#pragma once

typedef struct {
    int counter;
} atomic_t;

static inline void atomic_inc(atomic_t *v) {
    __asm__ __volatile__("lock; incl %0" : "+m"(v->counter));
}

static inline int atomic_dec_and_test(atomic_t *v) {
    unsigned char c;

    __asm__ __volatile__("lock; decl %0; sete %1" : "+m"(v->counter), "=qm"(c) : : "memory");
    return c != 0;
}

#define atomic_set(v, i) (((v)->counter) = (i))
#define atomic_get(v) ((v)->counter)
