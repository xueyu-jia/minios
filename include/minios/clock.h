#ifndef CLOCK_H
#define CLOCK_H

#include <minios/const.h>

//! NOTE: make PIT clock ms interval
#define HZ 100

extern int ticks;
extern u32 current_timestamp;

void clock_handler(int irq);
void init_clock();
int kern_get_ticks();
int sys_get_ticks();

#endif
