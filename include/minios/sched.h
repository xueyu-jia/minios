#pragma once

#include <minios/proc.h>
#include <minios/spinlock.h>

void rq_insert(process_t* proc);
void rq_remove(process_t* proc);

void sched();
void sched_init();
void sched_yield();

void wakeup(void* channel);
void wait_for_sem(void* chan, spinlock_t* lock);
void wakeup_for_sem(void* chan);
void wait_event(void* event);

void kern_nice(int inc);
void kern_set_rt(bool turn_rt);
void kern_rt_prio(int prio);

void kern_yield();
void kern_sleep(int ms);
