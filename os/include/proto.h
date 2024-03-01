
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
//#include <signal/signal.h>  //added by mingxuan 2021-2-28
#ifndef PROTO_H
#define PROTO_H
#include "type.h"
#include "const.h"
#include "signal.h"  //modified by mingxuan 2021-8-7
#include "msg.h" //added by yingchi 2022.01.07
#include "vfs.h"
#include "tty.h"
#include "proc.h"
#include "time.h"

// split all unnecessary part, functions defined in .asm & klib

/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC u32	in_dword(u16 port);             //read 32bit data from a port,qianglong
PUBLIC void	out_dword(u16 port, u32 value);//write 32bit data to a port,qianglong
// PUBLIC u32	in_mem_32(u32 phy_addr); 
// PUBLIC u32	out_mem_32(u32 phy_addr,u32 value);

// PUBLIC void	disp_str(char* info); // implement in tty 
// PUBLIC void	disp_color_str(char* info, int color);
PUBLIC void write_char(char ch, int pos);    //added by mingxuan 2019-5-19

//added by zcr
PUBLIC void	disable_irq(int irq);
PUBLIC void	enable_irq(int irq);
PUBLIC void	disable_int();
PUBLIC void	enable_int();
PUBLIC void	port_read(u16 port, void* buf, int n);
PUBLIC void	port_write(u16 port, void* buf, int n);
//~zcr

/* protect.c */
// PUBLIC void	init_prot();


/* klib.c */
PUBLIC void disp_int(int input);
PUBLIC void	delay(int time);
PUBLIC u32 get_ring_level();

/* kernel.asm */
PUBLIC void  sys_call();    //int_handler
u32  read_cr2();			//add by visual 2016.5.9
u32  read_cr3();
void refresh_page_cache();  //add by visual 2016.5.12
void refresh_gdt();//add by sundong 2023.3.8
//void restart_int();
//void save_context();
void restart_initial();		//added by xw, 18/4/18
void restart_restore();		//added by xw, 18/4/20
void sched();				//added by xw, 18/4/18
void halt();                //added by xw, 18/6/11
// u32 get_arg(void *uesp, int order);	//added by xw, 18/6/18

/* ktest.c */
// void TestA();
// void TestB();
// void TestC();
void initial();

/* keyboard.c */
//added by mingxuan 2019-5-19
// PUBLIC void init_kb();
// PUBLIC void keyboard_read();

/* tty.c */
//added by mingxuan 2019-5-19
// PUBLIC void tty_write(TTY* tty, char* buf, int len);
// PUBLIC int  tty_read(TTY* tty, char* buf, int len);

/* shell.c */
//added by mingxuan 2019-5-19
// PUBLIC void shell(char* s);
// PUBLIC void init_shell();

/* printf.c */
//added by mingxuan 2019-5-19
PUBLIC  int     printf(const char *fmt, ...);

/* vsprintf.c */
//added by mingxuan 2019-5-19
PUBLIC  int     vsprintf(char *buf, const char *fmt, va_list args);

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);
void get_rtc_datetime(struct tm *time);
/***************************************************************
* 以下是系统调用相关函数的声明
****************************************************************/
/* syscall.asm */
/*
PUBLIC void  sys_call();                //int_handler
PUBLIC int   get_ticks();
PUBLIC int   get_pid();					//add by visual 2016.4.6
PUBLIC int   get_pid_byname(char *);
PUBLIC pthread_t  pthread_self();		//added by ZengHao & MaLinhan 21.12.23
PUBLIC void* kmalloc(int size);			//edit by visual 2016.5.9
PUBLIC void* kmalloc_4k();				//edit by visual 2016.5.9
// PUBLIC void* malloc(int size);			//edit by visual 2016.5.9
PUBLIC void* malloc_4k();				//edit by visual 2016.5.9
// PUBLIC int free(void *arg);				//edit by visual 2016.5.9
PUBLIC int free_4k(void* AdddrLin);		//edit by visual 2016.5.9
PUBLIC int fork();						//add by visual 2016.4.8
PUBLIC int pthread_create(void *arg);			//add by visual 2016.4.11
PUBLIC void udisp_int(int arg);		//add by visual 2016.5.16
PUBLIC void udisp_str(char* arg);	//add by visual 2016.5.16
// PUBLIC u32 execve(char* path);		//add by visual 2016.5.16
PUBLIC u32 execve(char* path, char *argv[], char *envp[]);     //added by xyx&&wjh 2021.12.31
PUBLIC u32 execvp(char* file, char *argv[]);    //added by xyx&&wjh 2021.12.31
PUBLIC u32 execv(char* path, char *argv[]);     //added by xyx&&wjh 2021.12.31
PUBLIC void yield();				//added by xw, 18/4/19
PUBLIC void sleep(int n);			//added by xw, 18/4/19
PUBLIC void print_E();
PUBLIC void print_F();
PUBLIC u32 toatl_mem_size();           //added by wang 2021.8.26
PUBLIC int shmget(void *uesp);              //added by xiaofeng 2021-9-8
PUBLIC void *_shmat(void *uesp);             //added by xiaofeng 2021-9-8
PUBLIC void _shmdt(void *uesp);               //added by xiaofeng 2021-9-8
PUBLIC struct ipc_shm *shmctl(void *uesp);  //added by xiaofeng 2021-9-8
PUBLIC void *shmmemcpy(void *uesp);         //added by xiaofeng 2021-9-8
PUBLIC int pthread_mutex_init (pthread_mutex_t *mutex, pthread_mutexattr_t *mutexattr);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int pthread_mutex_destroy(pthread_mutex_t *mutex);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int pthread_mutex_lock (pthread_mutex_t *mutex);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int pthread_mutex_unlock (pthread_mutex_t *mutex);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int pthread_mutex_trylock(pthread_mutex_t *mutex);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int pthread_cond_init(pthread_cond_t *cond,const pthread_condattr_t *cond_attr);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int pthread_cond_wait(pthread_cond_t *cond,pthread_mutex_t *mutex);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int pthread_cond_timewait(pthread_cond_t *cond,pthread_mutex_t *mutex,int timeout);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int pthread_cond_signal(pthread_cond_t *cond);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int pthread_cond_broadcast(pthread_cond_t *cond);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int pthread_cond_destroy(pthread_cond_t *cond);//added by ZengHao & MaLinhan 2021.12.23
*/

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
int open(const char* pathname, int flags, int mode);
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
PUBLIC PROCESS* alloc_PCB();
PUBLIC void free_PCB(PROCESS *p);
PUBLIC void sys_yield();
//PUBLIC void sys_sleep(int n); //deleted by mingxuan 2021-8-13
PUBLIC void sys_sleep(); //modified by mingxuan 2021-8-13
PUBLIC void sys_wakeup(void *channel);
PUBLIC int ldt_seg_linear(PROCESS *p, int idx);
PUBLIC void* va2la(int pid, void* va);

PUBLIC void wait_for_sem(void *chan, struct spinlock *lk);
PUBLIC void wakeup_for_sem(void *chan);//modified by cjj 2021-12-23
PUBLIC void wait_event(void* event);

/* testfunc.c */
/*  //deleted by mingxuan 2021-8-13
PUBLIC void sys_print_E();
PUBLIC void sys_print_F();
*/

/*exec.c*/
//PUBLIC u32 sys_execve(char* path);		//add by visual 2016.5.23
PUBLIC u32 sys_execvp();      //added by xyx&&wjh 2021.12.31
PUBLIC u32 sys_execv();       //added by xyx&&wjh 2021.12.31
PUBLIC u32 sys_execve();    //modified by mingxuan 2021-8-11
//PUBLIC u32 do_execve(char *path);     //added by mingxuan 2021-8-11

/*fork.c*/
PUBLIC int sys_fork();					//add by visual 2016.5.25

/*wait.c*/
PUBLIC int sys_wait();                 //added by mingxuan 2021-1-6
/*exit.c*/
//PUBLIC void sys_exit(int status);       //added by mingxuan 2021-1-6
PUBLIC void sys_exit();         //modified by mingxuan 2021-8-13

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
PUBLIC	u32 init_page_pte(u32 pid);	//edit by visual 2016.4.28
PUBLIC 	void page_fault_handler(u32 vec_no, u32 err_code, u32 eip, u32 cs, u32 eflags);//add by visual 2016.4.19
PUBLIC	u32 get_pde_index(u32 AddrLin);//add by visual 2016.4.28
PUBLIC 	u32 get_pte_index(u32 AddrLin);
PUBLIC 	u32 get_pde_phy_addr(u32 pid);
PUBLIC 	u32 get_pte_phy_addr(u32 pid,u32 AddrLin);
PUBLIC  u32 get_page_phy_addr(u32 pid,u32 AddrLin);//线性地址
PUBLIC 	u32 pte_exist(u32 PageTblAddrPhy,u32 AddrLin);
PUBLIC 	u32 phy_exist(u32 PageTblPhyAddr,u32 AddrLin);
PUBLIC 	void write_page_pde(u32 PageDirPhyAddr,u32	AddrLin,u32 TblPhyAddr,u32 Attribute);
PUBLIC  void write_page_pte(	u32 TblPhyAddr,u32	AddrLin,u32 PhyAddr,u32 Attribute);
PUBLIC  u32 vmalloc(u32 size);
PUBLIC  int lin_mapping_phy(u32 AddrLin,u32 phy_addr,u32 pid,u32 pde_Attribute,u32 pte_Attribute);//edit by visual 2016.5.19
PUBLIC	void clear_kernel_pagepte_low();		//add by visual 2016.5.12

/*memman.c*/
PUBLIC u32 phy_kmalloc(u32 size);
PUBLIC u32 phy_kfree(u32 phy_addr);
PUBLIC u32 kern_kmalloc(u32 size);
PUBLIC u32 kern_kfree(u32 addr);
PUBLIC u32 phy_kmalloc_4k();
PUBLIC u32 phy_kfree_4k(u32 phy_addr);
PUBLIC u32 kern_kmalloc_4k();
PUBLIC u32 kern_kfree_4k(u32 addr);
PUBLIC u32 phy_malloc_4k();
PUBLIC u32 phy_free_4k(u32 phy_addr);
PUBLIC int ker_umalloc_4k(u32 AddrLin, u32 pid, u32 pte_attribute);
PUBLIC int ker_ufree_4k(u32 pid, u32 AddrLin);

/*mount.c */
PUBLIC int sys_mount();
PUBLIC int sys_umount();


/*slab.c*/
void *kmalloc(u32 size);
int kfree(u32 object);

PUBLIC int sys_get_time();
#endif