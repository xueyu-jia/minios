
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
//#include <signal/signal.h>  //added by mingxuan 2021-2-28
#ifndef PROTO_H
#define PROTO_H
#include <kernel/type.h>
#include <kernel/const.h>
#include <kernel/signal.h>  //modified by mingxuan 2021-8-7
#include <kernel/msg.h> //added by yingchi 2022.01.07
#include <kernel/vfs.h>
#include <kernel/tty.h>
#include <kernel/proc.h>
#include <kernel/time.h>
#include <kernel/x86.h>

// split all unnecessary part, functions defined in .asm & klib

/* klib.asm */
#include <kernel/x86-asm.h>

/* kernel.asm */
void refresh_page_cache();  //add by visual 2016.5.12
void refresh_gdt();//add by sundong 2023.3.8
void restart_initial();		//added by xw, 18/4/18
void restart_restore();		//added by xw, 18/4/20
void sched();				//added by xw, 18/4/18

void initial();


/* i8259.c */
#include <kernel/interrupt_x86.h>
// PUBLIC void put_irq_handler(int irq, irq_handler handler);
// PUBLIC void spurious_irq(int irq);

/* clock.c */


/*syscall.c*/   //added by zhenhao 2023.3.5
u32 get_arg(int order);
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
int wait();
void exit(int status);
int _signal(int sig, void* handler, void* _Handler);    //"user/ulib/signal.c" 中提供了上层封装
int sigsend(int pid, Sigaction* sigaction_p);
void sigreturn(int ebp);
u32 total_mem_size();
int shmget(int key, int size, int shmflg);
void* _shmat(int shmid, char* shmaddr, int shmflg);		//"user/ulib/ushm.c" 中提供了上层封装
void _shmdt(char* shmaddr);								//"user/ulib/ushm.c" 中提供了上层封装
struct ipc_shm* shmctl(int shmid, int cmd, struct ipc_shm* buf);
void* shmmemcpy(void* dst, const void* src, long unsigned int len);
int ftok(char* f, int key);
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
int pthread_mutex_unlock(pthread_mutex_t* mutex);
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
int stat(const char *pathname, struct stat* statbuf);

/* syscallc.c */		//edit by visual 2016.4.6
PUBLIC int   sys_get_ticks();           /* sys_call */
PUBLIC int   sys_get_pid();				//add by visual 2016.4.6
PUBLIC int   sys_get_pid_byname();
//PUBLIC void* sys_kmalloc(int size);			//edit by visual 2016.5.9   //deleted by mingxuan 2021-8-21
//PUBLIC void* sys_kmalloc_4k();				//edit by visual 2016.5.9   //deleted by mingxuan 2021-8-21
// PUBLIC void* sys_malloc(int size);			//edit by visual 2016.5.9   //deleted by zhenhao 2023.3.5

//PUBLIC void* sys_malloc_4k();				//edit by visual 2016.5.9
//PUBLIC u32 sys_malloc_4k(u32 vaddr);      //modified by mingxuan 2021-3-25
PUBLIC u32 sys_malloc_4k();                 //modified by mingxuan 2021-8-13

// PUBLIC int sys_free(void *arg);				//edit by visual 2016.5.9   //deleted by zhenhao 2023.3.5
PUBLIC int sys_free_4k();		//edit by visual 2016.5.9


//PUBLIC void sys_udisp_int(int arg);		//add by visual 2016.5.16
PUBLIC void sys_udisp_int();       //modified by mingxuan 2021-8-13
//PUBLIC void sys_udisp_str(char* arg);		//add by visual 2016.5.16
PUBLIC void sys_udisp_str();       //modified by mingxuan 2021-8-13

PUBLIC u32 sys_total_mem_size();            //modified by wang 2021.8.26
// PUBLIC void sys_test();//added by cjj 2021-12-25

/*pthread.c*/

PUBLIC int sys_pthread_create();		//add by visual 2016.4.11
PUBLIC pthread_t  sys_pthread_self();		//added by ZengHao & MaLinhan 21.12.23

PUBLIC void sys_pthread_exit();         //added by dongzhangqi 2023.5.4
PUBLIC int sys_pthread_join();

/* pthread_mutex.c pthread_cond.c */
PUBLIC int sys_pthread_mutex_init ();//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_mutex_destroy();//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_mutex_lock ();//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_mutex_unlock ();//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_mutex_trylock();//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_init();//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_wait();//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_timewait();//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_signal();//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_broadcast();//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_destroy();//added by ZengHao & MaLinhan 2021.12.23

/* proc.c */


/*exec.c*/
#include <kernel/exec.h>

/*fork.c*/
#include <kernel/fork.h>

/*wait.c*/
#include <kernel/wait.h>

/*exit.c*/
// #include <kernel/exit.h>

#include <kernel/interrupt_x86.h>

/* shm.c */
PUBLIC int sys_shmget();              //added by xiaofeng 2021-9-8
PUBLIC void *sys_shmat();             //added by xiaofeng 2021-9-8
PUBLIC void sys_shmdt();               //added by xiaofeng 2021-9-8
PUBLIC struct ipc_shm *sys_shmctl();  //added by xiaofeng 2021-9-8
PUBLIC void *sys_shmmemcpy();         //added by xiaofeng 2021-9-8

/*msg.c*/
PUBLIC int sys_ftok();
PUBLIC int sys_msgget();
PUBLIC int sys_msgsnd();
PUBLIC int sys_msgrcv();
PUBLIC int sys_msgctl();
void init_msgq();
/***************************************************************
* 以上是系统调用相关函数的声明
****************************************************************/

/*pagepte.c*/

/*memman.c*/


/*mount.c */
PUBLIC int sys_mount();
PUBLIC int sys_umount();


/*proc.c*/
PUBLIC void sys_nice();//lzq
PUBLIC void sys_set_rt();  //turn_rt is true,process was change to rt process;
                                //false,process was change to not rt process
PUBLIC void sys_rt_prio();
PUBLIC void sys_get_proc_msg();
PUBLIC int sys_get_time();
#endif
