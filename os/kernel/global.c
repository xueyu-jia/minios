
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
// #include "fs_const.h"
// #include "hd.h"
// #include "fs.h"
// #include "fat32.h"	//added by mingxuan 2019-5-17
#include "vfs.h"	//added by mingxuan 2019-5-17
#include "ksignal.h"	//added by mingxuan 2021-2-28
//modified by mingxuan 2021-4-2
// #include "../include/const.h"
// #undef	EXTERN	//EXTERN could have been defined as extern in const.h
// #define	EXTERN	//redefine EXTERN as nothing
// #include "../include/type.h"
// // #include "../include/protect.h"
// #include "../include/proc.h"
// #include "global.h"
// #include "../include/proto.h"
// #include "../include/fs_const.h"
// #include "../include/hd.h"
// #include "../include/fs.h"
// #include "../include/fat32.h"	//added by mingxuan 2019-5-17
// #include "../include/vfs.h"	//added by mingxuan 2019-5-17
// #include "ksignal.h"	//added by mingxuan 2021-2-28

// #include "semaphore.h"
/* save the execution environment of each cpu, which doesn't belong to any process.
 * added by xw, 18/6/1
 */
// PUBLIC	PROCESS			cpu_table[NR_CPUS];

// PUBLIC	PROCESS			proc_table[NR_PCBS];										//edit by visual 2016.4.5	
int		kernel_initial;

// PUBLIC	struct 	Semaphore	proc_table_sem;

//PUBLIC	char			task_stack[STACK_SIZE_TOTAL]; //delete  by viusal 2016.4.28

// PUBLIC	TASK	task_table[NR_TASKS] = {//{TestA, STACK_SIZE_TASK, "TestA"},	//edit by visual 2016.4.5	//deleted by mingxuan 2019-5-19
// 										//{TestB, STACK_SIZE_TASK, "TestB"},	//deleted by mingxuan 2019-5-19
// 										//{TestC, STACK_SIZE_TASK, "TestC"},	//deleted by mingxuan 2019-5-19
// 										{hd_service, STACK_SIZE_TASK, "hd_service"},
// 										{task_tty, STACK_SIZE_TASK, "task_tty"}};	//added by xw, 18/8/27


// PUBLIC	irq_handler		irq_table[NR_IRQ];

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
														sys_pthread_create,		//add by visual 2016.4.11	
														sys_udisp_int,		//add by visual 2016.5.16 
														sys_udisp_str,		//add by visual 2016.5.16
														sys_execve,			//add by visual 2016.5.16
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
														sys_total_mem_size,  //edded by wang 2021-8-21

														sys_shmget, //added by xiaofeng 2021-9-8
														sys_shmat,	//added by xiaofeng 2021-9-8
														sys_shmdt,	//added by xiaofeng 2021-9-8
														sys_shmctl, //added by xiaofeng 2021-9-8
														sys_shmmemcpy,

														sys_ftok,			//added by yingchi 2021.12.22
														sys_msgget,			//added by yingchi 2021.12.22
														sys_msgsnd,			//added by yingchi 2021.12.22
														sys_msgrcv,			//added by yingchi 2021.12.22
														sys_msgctl,			//added by yingchi 2021.12.22
														sys_test,
														sys_execvp,			//added by xyx&&wjh 2021.12.31
														sys_execv,			//added by xyx&&wjh 2021.12.31
														
														sys_pthread_self,//add by ZengHao & MaLinhan 21.12.23
														sys_pthread_mutex_init,//add by ZengHao & MaLinhan 21.12.23
														sys_pthread_mutex_destroy,//add by ZengHao & MaLinhan 21.12.23
														sys_pthread_mutex_lock,//add by ZengHao & MaLinhan 21.12.23
														sys_pthread_mutex_unlock,//add by ZengHao & MaLinhan 21.12.23
														sys_pthread_mutex_trylock,//add by ZengHao & MaLinhan 21.12.23
														sys_pthread_cond_init,//add by ZengHao & MaLinhan 21.12.230
														sys_pthread_cond_wait,//add by ZengHao & MaLinhan 21.12.23
														sys_pthread_cond_signal,//add by ZengHao & MaLinhan 21.12.23
														sys_pthread_cond_timewait,//add by ZengHao & MaLinhan 21.12.23
														sys_pthread_cond_broadcast,//add by ZengHao & MaLinhan 21.12.23
														sys_pthread_cond_destroy,//add by ZengHao & MaLinhan 21.12.23
														sys_get_pid_byname,
														sys_mount,
														sys_umount,
														sys_init_block_dev,
														sys_pthread_exit, //add by dongzhangqi 2023.5.17
														sys_pthread_join,
														sys_init_char_dev,
														sys_get_time,
														};

// PUBLIC TTY tty_table[NR_CONSOLES];			//added by mingxuan 2019-5-19
// PUBLIC CONSOLE console_table[NR_CONSOLES];	//added by mingxuan 2019-5-19