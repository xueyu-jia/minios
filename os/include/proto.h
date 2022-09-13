
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
//#include <signal/signal.h>  //added by mingxuan 2021-2-28
#include "signal.h"  //modified by mingxuan 2021-8-7
#include "msg.h" //added by yingchi 2022.01.07
#include "spinlock.h"
/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC u32	in_dword(u16 port);             //read 32bit data from a port,qianglong
PUBLIC void	out_dword(u16 port, u32 value);//write 32bit data to a port,qianglong
PUBLIC u32	in_mem_32(u32 phy_addr); 
PUBLIC u32	out_mem_32(u32 phy_addr,u32 value);

PUBLIC void	disp_str(char* info);
PUBLIC void	disp_color_str(char* info, int color);
PUBLIC void write_char(char ch);    //added by mingxuan 2019-5-19

//added by zcr
PUBLIC void	disable_irq(int irq);
PUBLIC void	enable_irq(int irq);
PUBLIC void	disable_int();
PUBLIC void	enable_int();
PUBLIC void	port_read(u16 port, void* buf, int n);
PUBLIC void	port_write(u16 port, void* buf, int n);
//~zcr

/* protect.c */
PUBLIC void	init_prot();
PUBLIC u32	seg2phys(u16 seg);

/* klib.c */
PUBLIC void	delay(int time);

/* kernel.asm */
u32  read_cr2();			//add by visual 2016.5.9
u32  read_cr3();
void refresh_page_cache();  //add by visual 2016.5.12
//void restart_int();
//void save_context();
void restart_initial();		//added by xw, 18/4/18
void restart_restore();		//added by xw, 18/4/20
void sched();				//added by xw, 18/4/18
void halt();                //added by xw, 18/6/11
u32 get_arg(void *uesp, int order);	//added by xw, 18/6/18

/* ktest.c */
void TestA();
void TestB();
void TestC();
void initial();

/* keyboard.c */
//added by mingxuan 2019-5-19
PUBLIC void init_kb();
PUBLIC void keyboard_read();

/* tty.c */
//added by mingxuan 2019-5-19
PUBLIC void in_process(TTY* p_tty,u32 key);
PUBLIC void task_tty();
PUBLIC void tty_write(TTY* tty, char* buf, int len);
PUBLIC int  tty_read(TTY* tty, char* buf, int len);

/* shell.c */
//added by mingxuan 2019-5-19
PUBLIC void shell(char* s);
PUBLIC void init_shell();

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

/***************************************************************
* 以下是系统调用相关函数的声明
****************************************************************/
/* syscall.asm */
PUBLIC void  sys_call();                /* int_handler */
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
// PUBLIC u32 exec(char* path);		//add by visual 2016.5.16
PUBLIC u32 exec(char* path, char *argv[], char *envp[]);     //added by xyx&&wjh 2021.12.31
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


/* syscallc.c */		//edit by visual 2016.4.6
PUBLIC int   sys_get_ticks();           /* sys_call */
PUBLIC int   sys_get_pid();				//add by visual 2016.4.6
PUBLIC int   sys_get_pid_byname(char*);
//PUBLIC void* sys_kmalloc(int size);			//edit by visual 2016.5.9   //deleted by mingxuan 2021-8-21
//PUBLIC void* sys_kmalloc_4k();				//edit by visual 2016.5.9   //deleted by mingxuan 2021-8-21
PUBLIC void* sys_malloc(int size);			//edit by visual 2016.5.9

//PUBLIC void* sys_malloc_4k();				//edit by visual 2016.5.9
//PUBLIC u32 sys_malloc_4k(u32 vaddr);      //modified by mingxuan 2021-3-25
PUBLIC u32 sys_malloc_4k();                 //modified by mingxuan 2021-8-13

PUBLIC int sys_free(void *arg);				//edit by visual 2016.5.9
PUBLIC int sys_free_4k(void* AdddrLin);		//edit by visual 2016.5.9


//PUBLIC void sys_udisp_int(int arg);		//add by visual 2016.5.16
PUBLIC void sys_udisp_int(void *uesp);       //modified by mingxuan 2021-8-13
//PUBLIC void sys_udisp_str(char* arg);		//add by visual 2016.5.16
PUBLIC void sys_udisp_str(void *uesp);       //modified by mingxuan 2021-8-13

PUBLIC u32 sys_total_mem_size();            //modified by wang 2021.8.26
PUBLIC void sys_test(int no);//added by cjj 2021-12-25

/*pthread.c*/

PUBLIC int sys_pthread_create(void *arg);		//add by visual 2016.4.11
PUBLIC pthread_t  sys_pthread_self();		//added by ZengHao & MaLinhan 21.12.23

/* pthread_mutex.c pthread_cond.c */
PUBLIC int sys_pthread_mutex_init (void* uesp);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_mutex_destroy(pthread_mutex_t *mutex);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_mutex_lock (pthread_mutex_t *mutex);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_mutex_unlock (pthread_mutex_t *mutex);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_mutex_trylock(pthread_mutex_t *mutex);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_init(void* uesp);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_wait(void* uesp);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_timewait(void* uesp);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_signal(pthread_cond_t *cond);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_broadcast(pthread_cond_t *cond);//added by ZengHao & MaLinhan 2021.12.23
PUBLIC int sys_pthread_cond_destroy(pthread_cond_t *cond);//added by ZengHao & MaLinhan 2021.12.23

/* proc.c */
PUBLIC PROCESS* alloc_PCB();
PUBLIC void free_PCB(PROCESS *p);
PUBLIC void sys_yield();
//PUBLIC void sys_sleep(int n); //deleted by mingxuan 2021-8-13
PUBLIC void sys_sleep(void *uesp); //modified by mingxuan 2021-8-13
PUBLIC void sys_wakeup(void *channel);
PUBLIC int ldt_seg_linear(PROCESS *p, int idx);
PUBLIC void* va2la(int pid, void* va);

PUBLIC void wait_for_sem(void *chan, struct spinlock *lk);
PUBLIC void wakeup_for_sem(void *chan);//modified by cjj 2021-12-23

/* testfunc.c */
/*  //deleted by mingxuan 2021-8-13
PUBLIC void sys_print_E();
PUBLIC void sys_print_F();
*/

/*exec.c*/
//PUBLIC u32 sys_exec(char* path);		//add by visual 2016.5.23
PUBLIC u32 sys_execvp(void *uesp);      //added by xyx&&wjh 2021.12.31
PUBLIC u32 sys_execv(void *uesp);       //added by xyx&&wjh 2021.12.31
PUBLIC u32 sys_exec(void *uesp);    //modified by mingxuan 2021-8-11
//PUBLIC u32 do_exec(char *path);     //added by mingxuan 2021-8-11

/*fork.c*/
PUBLIC int sys_fork();					//add by visual 2016.5.25

/*wait.c*/
PUBLIC int sys_wait();                 //added by mingxuan 2021-1-6
/*exit.c*/
//PUBLIC void sys_exit(int status);       //added by mingxuan 2021-1-6
PUBLIC void sys_exit(void *uesp);         //modified by mingxuan 2021-8-13

/* shm.c */
PUBLIC int sys_shmget(void *uesp);              //added by xiaofeng 2021-9-8
PUBLIC void *sys_shmat(void *uesp);             //added by xiaofeng 2021-9-8
PUBLIC void sys_shmdt(void *uesp);               //added by xiaofeng 2021-9-8
PUBLIC struct ipc_shm *sys_shmctl(void *uesp);  //added by xiaofeng 2021-9-8
PUBLIC void *sys_shmmemcpy(void *uesp);         //added by xiaofeng 2021-9-8

/*msg.c*/
PUBLIC int sys_ftok(void* args);
PUBLIC int sys_msgget(void* args);
PUBLIC int sys_msgsnd(void* args);
PUBLIC int sys_msgrcv(void* args);
PUBLIC int sys_msgctl(void* args);
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
PUBLIC u32 phy_free_4k(u32 phy_addr);

/*mount.c */
PUBLIC int sys_mount(void *uesp);
PUBLIC int sys_umount(void *uesp);

/*fs.c*/
PUBLIC int sys_init_block_dev(void *uesp);