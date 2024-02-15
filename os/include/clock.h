#ifndef CLOCK_H
#define CLOCK_H
#include "const.h"

extern int	ticks;
extern u32 	current_timestamp;

PUBLIC void init_clock();
PUBLIC int sys_get_ticks();
#endif