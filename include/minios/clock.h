#pragma once

#include <klib/stdint.h>

//! NOTE: make PIT clock ms interval
#define HZ 100

extern int ticks;
extern u32 current_timestamp;

void clock_handler(int irq);
void init_clock();

int kern_getticks();
