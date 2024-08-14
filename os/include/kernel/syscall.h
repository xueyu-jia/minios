#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#define _NR_get_ticks       0
#define _NR_get_pid         1
#define _NR_malloc_4k       2
#define _NR_free_4k         3
#define _NR_fork            4
#define _NR_pthread_create  5
#define _NR_udisp_int     	6
#define _NR_udisp_str     	7
#define _NR_execve     		8
#define _NR_yield			9
#define _NR_sleep			10
#define _NR_open			11
#define _NR_close			12
#define _NR_read			13
#define _NR_write			14
#define _NR_lseek			15
#define _NR_unlink			16
#define _NR_creat			17
#define _NR_closedir 		18
#define _NR_opendir 		19
#define _NR_mkdir  			20
#define _NR_rmdir   		21
#define _NR_readdir         22
#define _NR_chdir           23
#define _NR_getcwd          24
#define _NR_wait			25
#define _NR_exit			26
#define _NR_signal			27
#define _NR_sigsend			28
#define _NR_sigreturn		29
#define _NR_total_mem_size  30
#define _NR_shmget          31
#define _NR_shmat           32
#define _NR_shmdt           33
#define _NR_shmctl          34
#define _NR_shmmemcpy       35
#define _NR_ftok			36
#define _NR_msgget			37
#define _NR_msgsnd			38
#define _NR_msgrcv			39
#define _NR_msgctl			40
#define _NR_test			41
#define _NR_execvp			42
#define _NR_execv			43
#define _NR_pthread_self	44
#define _NR_pthread_mutex_init    		45
#define _NR_pthread_mutex_destroy 		46
#define _NR_pthread_mutex_lock    		47
#define _NR_pthread_mutex_unlock  		48
#define _NR_pthread_mutex_trylock 		49
#define _NR_pthread_cond_init 	  		50
#define _NR_pthread_cond_wait 	  		51
#define _NR_pthread_cond_signal 	  	52
#define _NR_pthread_cond_timewait 	  	53
#define _NR_pthread_cond_broadcast 	  	54
#define _NR_pthread_cond_destroy	  	55
#define _NR_get_pid_byname				56
#define _NR_mount						57
#define _NR_umount						58
#define _NR_pthread_exit                59
#define _NR_pthread_join                60
#define _NR_get_time					61
#define _NR_stat						62
#define _NR_nice		 				63 	// added by zq
#define _NR_set_rt		 				64
#define _NR_rt_prio		 				65
#define _NR_get_proc_msg				66 
#define INT_VECTOR_SYS_CALL             0x90

/* 无参数的系统调用 */
#define _syscall0(NR_syscall) ({		\
	int _retval;					        \
	asm volatile (						\
		"int $0x90"						\
		: "=a" (_retval)					\
		: "a" (NR_syscall)				\
		: "cc", "memory"				\
	);							    	\
	_retval;						       	\
})

/* 一个参数的系统调用 */
#define _syscall1(NR_syscall, ARG1) ({		\
	int _retval;					            \
	asm volatile (					       	\
		"int $0x90"						    \
		: "=a" (_retval)					    \
		: "a" (NR_syscall), "b" (ARG1)		\
		: "cc", "memory"					\
	);							       		\
	_retval;						       		\
})

/* 两个参数的系统调用 */
#define _syscall2(NR_syscall, ARG1, ARG2) ({	\
	int _retval;						       		\
	asm volatile (					       		\
	"int $0x90"						       		\
		: "=a" (_retval)					       	\
		: "a" (NR_syscall), "b" (ARG1), "c" (ARG2)	\
		: "cc", "memory"						\
	);							       			\
	_retval;						       			\
})

/* 三个参数的系统调用 */
#define _syscall3(NR_syscall, ARG1, ARG2, ARG3) ({	\
	int _retval;						       			\
	asm volatile (					       			\
	"int $0x90"					       				\
		: "=a" (_retval)					       		\
		: "a" (NR_syscall), "b" (ARG1), "c" (ARG2), "d" (ARG3)  \
		: "cc", "memory"					       	\
	);							       				\
	_retval;						       				\
})

/* 四个参数的系统调用 */
#define _syscall4(NR_syscall, ARG1, ARG2, ARG3, ARG4) ({	\
	int _retval;						       					\
	asm volatile (					       					\
		"int $0x90"					       					\
		: "=a" (_retval)					       				\
		: "a" (NR_syscall), "b" (ARG1), "c" (ARG2), "d" (ARG3), "S" (ARG4)	\
		: "cc", "memory"					       			\
	);							       						\
	_retval;						       						\
})

/* 五个参数的系统调用 */
#define _syscall5(NR_syscall, ARG1, ARG2, ARG3, ARG4, ARG5) ({	\
	int _retval;						       						\
	asm volatile (					       						\
		"int $0x90"					       						\
		: "=a" (_retval)					       					\
		: "a" (NR_syscall), "b" (ARG1), "c" (ARG2), "d" (ARG3), "S" (ARG4), "D" (ARG5)	\
		: "cc", "memory"					       				\
	);							       							\
	_retval;						       							\
})

#endif