#pragma once

#include <klib/stdint.h>
#include <uapi/minios/time.h>

//! NOTE: make PIT clock ms interval
#define HZ 100

extern int ticks;
extern u32 current_timestamp;
extern double tsc_ns_freq;

void clock_handler(int irq);
void init_clock();
void early_clock_sync();

int kern_getticks();
clock_t kern_clock();
