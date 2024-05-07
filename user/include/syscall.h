#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

#include "type.h"
#include "const.h"
#include "time.h"
#include "fcntl.h"
#include "stat.h"
#include "../include/signal.h"
#include "../include/ushm.h"
#include "../include/msg.h"
#include "../include/pthread.h"

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

typedef struct process_message
{
	u32 pid;
	int nice;
	double vruntime;
	u64 sum_cpu_use;
}proc_msg;

int get_ticks();
int get_pid();
void* malloc_4k();
int free_4k(void* AdddrLin);
int fork();
int pthread_create(int* thread, void* attr, void* entry, void* arg);
void udisp_int(int arg);
void udisp_str(char* arg);
u32 execve(char* path, char* argv[], char* envp[]);
void yield();
void sleep(int n);
int open(const char* pathname, int flags, ...);
int close(int fd);
int read(int fd, void* buf, int count);
int write(int fd, const void* buf, int count);
int lseek(int fd, int offset, int whence);
int unlink(const char* pathname);
int creat(const char* pathname);
DIR* opendir(const char* dirname);
int closedir(DIR* dirp);
int mkdir(const char* dirname, int mode);
int rmdir(const char* dirname);
struct dirent* readdir(DIR* dirp);
int chdir(const char* path);
char* getcwd(char* buf, int size);
int wait(int *status);
void exit(int status);
int _signal(int sig, void* handler, void* _Handler);	//"user/ulib/signal.c" 中提供了上层封装
int sigsend(int pid, Sigaction* sigaction_p);
void sigreturn(int ebp);
u32 total_mem_size();
int shmget(int key, int size, int shmflg);
void* _shmat(int shmid, char* shmaddr, int shmflg);		//"user/ulib/ushm.c" 中提供了上层封装
void _shmdt(char* shmaddr);								//"user/ulib/ushm.c" 中提供了上层封装
struct ipc_shm* shmctl(int shmid, int cmd, struct ipc_shm* buf);
void* shmmemcpy(void* dst, const void* src, long unsigned int len);
int ftok(char* f,int key);
int msgget(key_t key, int msgflg);
int msgsnd(int msqid, const void* msgp, int msgsz, int msgflg);
int msgrcv(int msqid, void* msgp, int msgsz, long msgtyp, int msgflg);
int msgctl(int msgqid, int cmd, msqid_ds* buf);
void test(int no);
u32 execvp(char* file, char* argv[]);
u32 execv(char* path, char* argv[]);
pthread_t pthread_self();
int pthread_mutex_init(pthread_mutex_t* mutex, pthread_mutexattr_t* mutexattr);
int pthread_mutex_destroy(pthread_mutex_t* mutex);
int pthread_mutex_lock(pthread_mutex_t* mutex);
int pthread_mutex_unlock (pthread_mutex_t* mutex);
int pthread_mutex_trylock(pthread_mutex_t* mutex);
int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* cond_attr);
int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);
int pthread_cond_timewait(pthread_cond_t* cond, pthread_mutex_t* mutex, int* timeout);
int pthread_cond_signal(pthread_cond_t* cond);
int pthread_cond_broadcast(pthread_cond_t* cond);
int pthread_cond_destroy(pthread_cond_t* cond);
int get_pid_byname(char* name);
int mount(const char *source, const char *target,const char *filesystemtype, unsigned long mountflags, const void *data);
int umount(const char *target);
void pthread_exit(void *retval);
int pthread_join(pthread_t pthread, void **retval);
int get_time(struct tm* time);
int stat(const char *pathname, struct stat* statbuf);
void nice(int val);
void set_rt(int turn_rt);
void rt_prio(int prio);
void get_proc_msg(proc_msg* msg);

#endif