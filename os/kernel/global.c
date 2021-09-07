
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
// #define GLOBAL_VARIABLES_HERE
/*
#include "const.h"
#undef	EXTERN	//EXTERN could have been defined as extern in const.h
#define	EXTERN	//redefine EXTERN as nothing
#include "type.h"
#include "protect.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "fs_const.h"
#include "hd.h"
#include "fs.h"
#include "fat32.h"	//added by mingxuan 2019-5-17
#include "vfs.h"	//added by mingxuan 2019-5-17
#include "./signal/ksignal.h"	//added by mingxuan 2021-2-28
*/
//modified by mingxuan 2021-4-2
#include "../include/const.h"
#undef	EXTERN	//EXTERN could have been defined as extern in const.h
#define	EXTERN	//redefine EXTERN as nothing
#include "../include/type.h"
#include "../include/protect.h"
#include "../include/proc.h"
#include "../include/global.h"
#include "../include/proto.h"
#include "../include/fs_const.h"
#include "../include/hd.h"
#include "../include/fs.h"
#include "../include/fat32.h"	//added by mingxuan 2019-5-17
#include "../include/vfs.h"	//added by mingxuan 2019-5-17
#include "../include/ksignal.h"	//added by mingxuan 2021-2-28

/* save the execution environment of each cpu, which doesn't belong to any process.
 * added by xw, 18/6/1
 */
PUBLIC	PROCESS			cpu_table[NR_CPUS];

PUBLIC	PROCESS			proc_table[NR_PCBS];										//edit by visual 2016.4.5	

//PUBLIC	char			task_stack[STACK_SIZE_TOTAL]; //delete  by viusal 2016.4.28

PUBLIC	TASK	task_table[NR_TASKS] = {//{TestA, STACK_SIZE_TASK, "TestA"},	//edit by visual 2016.4.5	//deleted by mingxuan 2019-5-19
										//{TestB, STACK_SIZE_TASK, "TestB"},	//deleted by mingxuan 2019-5-19
										//{TestC, STACK_SIZE_TASK, "TestC"},	//deleted by mingxuan 2019-5-19
										{hd_service, STACK_SIZE_TASK, "hd_service"},
										{task_tty, STACK_SIZE_TASK, "task_tty"}};	//added by xw, 18/8/27


PUBLIC	irq_handler		irq_table[NR_IRQ];

PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {	sys_get_ticks, 									//1st
														sys_get_pid,		//add by visual 2016.4.6
														
														//删除了4个系统调用接口：kmalloc、kmalloc_4k、malloc和free, mingxuan 2021-3-25
														//sys_kmalloc,		//add by visual 2016.4.6 	//deleted by mingxuan 2021-3-25
														//sys_kmalloc_4k,	//add by visual 2016.4.7	//deleted by mingxuan 2021-3-25
														//sys_malloc,		//add by visual 2016.4.7	
														sys_malloc_4k,		//add by visual 2016.4.7 
														//sys_free,			//add by visual 2016.4.7 
														sys_free_4k,		//add by visual 2016.4.7 

														sys_fork,			//add by visual 2016.4.8 	//5th
														sys_pthread,		//add by visual 2016.4.11	
														sys_udisp_int,		//add by visual 2016.5.16 
														sys_udisp_str,		//add by visual 2016.5.16
														sys_exec,			//add by visual 2016.5.16
														sys_yield,			//added by xw				//10th
													    sys_sleep,			//added by xw				
													    
														//sys_print_E,		//added by xw	//deleted by mingxuan 2021-8-13
													    //sys_print_F,		//added by xw	//deleted by mingxuan 2021-8-13
														
														sys_open,			//added by xw, 18/6/18
													    sys_close,			//added by xw, 18/6/18		
													    sys_read,			//added by xw, 18/6/18		
													    sys_write,			//added by xw, 18/6/18		//15th
													    sys_lseek,			//added by xw, 18/6/18
														sys_unlink,			//added by xw, 18/6/19		

														sys_create,			//added by mingxuan 2019-5-17	
														sys_delete,			//added by mingxuan 2019-5-17
														sys_opendir,		//added by mingxuan 2019-5-17	//20th
														sys_createdir,		//added by mingxuan 2019-5-17
														sys_deletedir,		//added by mingxuan 2019-5-17
                                                        sys_readdir,		//added by pg999w 2021-1-10		
														sys_chdir,          //added by ran
														sys_getcwd,         //added by ran					//25th

														sys_wait, 			//added by mingxuan 2021-1-6
														sys_exit,			//added by mingxuan 2021-1-6

														sys_signal,			//added by mingxuan 2021-2-28	
														sys_sigsend,		//added by mingxuan 2021-2-28
														sys_sigreturn,		//added by mingxuan 2021-2-28	//30th
														sys_total_mem_size  //edded by wang 2021-8-21
														};

PUBLIC TTY tty_table[NR_CONSOLES];			//added by mingxuan 2019-5-19
PUBLIC CONSOLE console_table[NR_CONSOLES];	//added by mingxuan 2019-5-19