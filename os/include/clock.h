#ifndef CLOCK_H
#define CLOCK_H
#include "const.h"

extern int	ticks;

PUBLIC void init_clock();
PUBLIC int sys_get_ticks();
#endif