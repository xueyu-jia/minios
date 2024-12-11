#ifndef CLOCK_H
#define CLOCK_H

#include <kernel/const.h>

//! NOTE: make PIT clock ms interval
#define HZ 100

extern int ticks;
extern u32 current_timestamp;

PUBLIC void clock_handler(int irq);
PUBLIC void init_clock();
PUBLIC int kern_get_ticks();
PUBLIC int sys_get_ticks();

#endif
