
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/* 
 * To make things more direct. In the headers below, if a variable declaration has a EXTERN prefix,
 * the variable will be defined here.
 * added by xw, 18/6/17
 */
// THE SKILL MAKES VARIABLES COUPLING BETWEEN MODULES
// #define GLOBAL_VARIABLES_HERE

// #undef	EXTERN	//EXTERN could have been defined as extern in const.h
// #define	EXTERN	//redefine EXTERN as nothing
#include "const.h"
#include "type.h"
#include "global.h"
#include "proto.h"
#include "fs.h"	//added by mingxuan 2019-5-17
#include "ksignal.h"	
										//edit by visual 2016.4.5	
int		kernel_initial;