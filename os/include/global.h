
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */

/* equal to 1 if kernel is initializing, equal to 0 if done.
 * added by xw, 18/5/31
 */
#include "type.h"
extern	int		kernel_initial;

extern	int		ticks; // defined in clock.c
