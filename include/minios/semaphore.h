#pragma once

#include <minios/spinlock.h>

typedef struct {
    int value;
    int maxValue;
    int active;
    spinlock_t lock;
} semaphore_t;

int ksem_init(semaphore_t *sem, int max);
int ksem_destroy(semaphore_t *sem);
int ksem_wait(semaphore_t *sem, int count);
int ksem_post(semaphore_t *sem, int count);
int ksem_trywait(semaphore_t *sem, int count);
int ksem_getvalue(semaphore_t *sem);

extern semaphore_t proc_table_sem;
