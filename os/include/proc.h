
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#ifndef KERNEL_PROC_H
#define KERNEL_PROC_H

#include "fs.h" //added by ran
#include "pthread.h"
#include "protect.h"

/* Used to find saved registers in the new kernel stack,
 * for there is no variable name you can use in C.
 * The macro defined below must coordinate with the equivalent in sconst.inc,
 * whose content is used in asm.
 * Note that you shouldn't use saved registers in the old
 * kernel stack, i.e. STACK_FRAME, any more.
 * To access them, use a pointer plus a offset defined
 * below instead.
 * added by xw, 17/12/11
 */
#define INIT_STACK_SIZE 1024 * 8	//new kernel stack is 8kB
#define P_STACKBASE 0
#define GSREG 0
#define FSREG 1 * 4
#define ESREG 2 * 4
#define DSREG 3 * 4
#define EDIREG 4 * 4
#define ESIREG 5 * 4
#define EBPREG 6 * 4
#define KERNELESPREG 7 * 4
#define EBXREG 8 * 4
#define EDXREG 9 * 4
#define ECXREG 10 * 4
#define EAXREG 11 * 4
#define RETADR 12 * 4
#define ERROR 13 * 4   //added by lcy 2023.10.26
#define EIPREG 14 * 4
#define CSREG 15 * 4
#define EFLAGSREG 16 * 4
#define ESPREG 17 * 4
#define SSREG 18 * 4
#define P_STACKTOP 19 * 4

/*总PCB表数和taskPCB表数*/
//modified by xw, 18/8/27
//the memory space we put kernel is 0x30400~0x6ffff, so we should limit kernel size
// #define NR_PCBS	32		//add by visual 2016.4.5
// #define NR_K_PCBS 10		//add by visual 2016.4.5
#define NR_PCBS		64								//modified by zhenhao 2023.3.5
//#define NR_TASKS	4	//TestA~TestC + hd_service //deleted by mingxuan 2019-5-19
#define NR_TASKS	3	//task_tty + hd_service		//modified by mingxuan 2019-5-19
#define NR_K_PCBS	16								//modified by zhenhao 2023.3.5
#define PID_INIT NR_K_PCBS
#define PID_NO_PROC	1000
#define PROC_READY_MAX	30	//xiaofeng
#define PROC_NICE_MAX	19

//~xw

#define NR_CPUS		1		//numbers of cpu. added by xw, 18/6/1
#define	NR_FILES	64		//numbers of files a process can own. added by xw, 18/6/14

#define NR_SIGNALS  32		//number of signals	//added by mingxuan 2021-2-28

//enum proc_stat	{IDLE,READY,WAITING,RUNNING};		//add by visual smile 2016.4.5
//enum proc_stat	{IDLE,READY,SLEEPING};		//eliminate RUNNING state
//enum proc_stat	{IDLE,READY,SLEEPING,KILLED};
												/* add KILLED state. when a process's state is KILLED, the process
												 * won't be scheduled anymore, but all of the resources owned by
												 * it is not freed yet.
												 * added by xw, 18/12/19
												 */
//enum proc_stat	{IDLE,READY,SLEEPING,KILLED,FREE,WAKE};	//add FREE state which means this PCB is free, added by mingxuan 2021-9-21

// enum proc_stat	{IDLE,READY,SLEEPING,KILLED,FREE,WAKE,WAITING,ZOMBY};//combine wai_flag into proc_stat 2023.6.2 changed by dongzhangqi
//??? why so many duplicated PCB state
//	FREE: 此PCB空闲未分配
//	READY: 此PCB表示的进程/线程正在使用且可被调度
//	SLEEPING: 此PCB表示的进程/线程处于睡眠等待状态
//	KILLED: 
enum proc_stat	{FREE,READY,SLEEPING,KILLED}; //simplify 20240314 jiangfeng

// enum wait_exit_flag	{NORMAL};    		//used for exit and wait //added by mingxuan 2021-1-6
														//deleted by dongzhangqi 2023-6-2

#define NR_CHILD_MAX (NR_PCBS-NR_K_PCBS-1)    //定义最多子进程/线程数量	//add by visual 2016.5.26
#define TYPE_PROCESS	0//进程//add by visual 2016.5.26
#define TYPE_THREAD		1//线程//add by visual 2016.5.26
#define TIME_SCALE		100

typedef struct s_stackframe {	/* proc_ptr points here				↑ Low			*/
	u32	gs;			/* ┓						│			*/
	u32	fs;			/* ┃						│			*/
	u32	es;			/* ┃						│			*/
	u32	ds;			/* ┃						│			*/
	u32	edi;		/* ┃						│			*/
	u32	esi;		/* ┣ pushed by save()				│			*/
	u32	ebp;		/* ┃						│			*/
	u32	kernel_esp;	/* <- 'popad' will ignore it			│			*/
	u32	ebx;		/* ┃						↑栈从高地址往低地址增长*/
	u32	edx;		/* ┃						│			*/
	u32	ecx;		/* ┃						│			*/
	u32	eax;		/* ┛						│			*/
	u32	retaddr;	/* return address for assembly code save()	│			*/
	u32	error;		//add by lcy 2023.10.26
	u32	eip;		/*  ┓						│			*/
	u32	cs;			/*  ┃						│			*/
	u32	eflags;		/*  ┣ these are pushed by CPU during interrupt	│			*/
	u32	esp;		/*  ┃						│			*/
	u32	ss;			/*  ┛						┷High			*/
}STACK_FRAME;

typedef struct s_tree_info{//进程树记录，包括父进程，子进程，子线程  //add by visual 2016.5.25
	int type;			//当前是进程还是线程
	int real_ppid;  	//亲父进程，创建它的那个进程
	int ppid;			//当前父进程
	int child_p_num;	//子进程数量
	int child_process[NR_CHILD_MAX];//子进程列表
	int child_t_num;		//子线程数量
	int child_thread[NR_CHILD_MAX];//子线程列表
	int text_hold;			//是否拥有代码
	int data_hold;			//是否拥有数据
}TREE_INFO;

typedef struct s_lin_memmap {//线性地址分布结构体	edit by visual 2016.5.25
	u32 text_lin_base;						//代码段基址
	u32 text_lin_limit;						//代码段界限

	u32 data_lin_base;						//数据段基址
	u32 data_lin_limit;						//数据段界限

	u32 vpage_lin_base;						//保留内存基址
	u32 vpage_lin_limit;					//保留内存界限

	u32 heap_lin_base;						//堆基址
	u32 heap_lin_limit;						//堆界限

	u32 stack_lin_base;						//栈基址
	u32 stack_lin_limit;					//栈界限（使用时注意栈的生长方向）

	u32 arg_lin_base;						//参数内存基址
	u32 arg_lin_limit;						//参数内存界限

	u32 kernel_lin_base;					//内核基址
	u32 kernel_lin_limit;					//内核界限

	u32 stack_child_limit;					//分给子线程的栈的界限		//add by visual 2016.5.27
}LIN_MEMMAP;

/*注意：sconst.inc文件中规定了变量间的偏移值，新添变量不要破坏原有顺序结构*/
typedef struct s_proc {
	//STACK_FRAME regs;          /* process registers saved in stack frame */ deleted by lcy 2023.10.25

	u16 ldt_sel;               /* gdt selector giving ldt base and limit */
	DESCRIPTOR ldts[LDT_SIZE]; /* local descriptors for code and data */

	STACK_FRAME* esp_save_int;
	//char* esp_save_int;		//to save the position of esp in the kernel stack of the process
							//added by xw, 17/12/11
//	char* esp_save_syscall;	//to save the position of esp in the kernel stack of the process
	char* esp_save_context;	//to save the position of esp in the kernel stack of the process
//	int   save_type;		//the cause of process losting CPU	//save_type is not needed any more, xw, 18/4/20
							//1st-bit for interruption, 2nd-bit for context, 3rd-bit for syscall
//	char* esp_save_syscall_arg;	// to save the position of esp in the kernel stack of the process, used in save_syscall
								//added by zhenhao, 2023.3.6
	void* channel;			/*if non-zero, sleeping on channel, which is a pointer of the target field
							for example, as for syscall sleep(int n), the target field is 'ticks',
							and the channel is a pointer of 'ticks'.
							*/

	LIN_MEMMAP	memmap;			//线性内存分部信息 		add by visual 2016.5.4
	TREE_INFO info;				//记录进程树关系	add by visual 2016.5.25
    int ticks;                 /* remained ticks */
    int priority;

	u32 pid;                   /* process id passed in from MM */
	pthread_t pthread_id;		//线程id Add By ZengHao & MaLinhan 21.12.22

	char p_name[16];           /* name of the process */

	enum proc_stat stat;			//add by visual 2016.4.5

	u32 cr3;						//add by visual 2016.4.5

	struct vfs_dentry* cwd;

	int suspended; 				//线程id Add By ZengHao & MaLinhan 21.12.22
	//added by zcr
	struct file_desc * filp[NR_FILES];
	//~zcr

	int exit_status;				//added by mingxuan 2021-1-6
	int child_exit_status;			//added by mingxuan 2021-1-6

	//与signal相关
	//added by mingxuan 2021-2-28
	void* sig_handler[NR_SIGNALS];
	u32 sig_set;
	u32 sig_arg[NR_SIGNALS];
	void* _Hanlder;

	//add by dongzhangqi 2023.5.8
	//线程相关
	pthread_attr_t attr;           
	void * retval;
	u32 who_wait_flag;   
	
	// cfs attr added by xiaofeng 
	int is_rt;			//flag for Real-time(T) and not-Real-time(F) process
    int rt_priority;	//priority for Real-time process 

	int nice;
	int weight; 		//priority for not-Real-time process 
	double vruntime;
	u32 cpu_use;
	u64 sum_cpu_use;


}PROCESS_0;

//new PROCESS struct with PCB and process's kernel stack
//added by xw, 17/12/11
typedef union task_union {
	PROCESS_0 task;
	char stack[INIT_STACK_SIZE/sizeof(char)];
}PROCESS;

typedef struct s_task {
	task_f	initial_eip;
	int ready;
	int rpl;
	int rt;
	int priority_nice; // rt priority if rt == 1, else nice
	int	stacksize;
	char	name[32];
}TASK;

/* stacks of tasks */
//#define STACK_SIZE_TESTA	0x8000	//delete by visual 2016.4.5
//#define STACK_SIZE_TESTB	0x8000
//#define STACK_SIZE_TESTC	0x8000

#define STACK_SIZE_TASK		0x1000	//add by visual 2016.4.5
//#define STACK_SIZE_TOTAL	(STACK_SIZE_TASK*NR_PCBS)	//edit by visual 2016.4.5
extern	u32 cr3_ready;
extern  int     u_proc_sum;
extern	PROCESS*	p_proc_current;
extern	PROCESS*	p_proc_next;
extern	PROCESS		cpu_table[];
extern	PROCESS		proc_table[];
extern  TASK	task_table[NR_TASKS];
extern	int		kernel_initial;
extern 	int 	nice_to_weight[];

#define proc2pid(x) (x - proc_table)
#define pid2proc(pid) ((PROCESS*)&proc_table[(pid)])
#define proc_kstacktop(proc) ((STACK_FRAME*)(((char*)((proc)+1) - P_STACKTOP)))

#define PROC_CONTEXT	10 * 4
//added by zq
typedef struct process_message
{
	u32 pid;
	int nice;
	double vruntime;
	u64 sum_cpu_use;
}proc_msg;

void sched_init();
void in_rq(PROCESS* p_in);
void out_rq(PROCESS* p_out);
PUBLIC void idle();
void proc_update();

//added by jiangfeng
#define proc_real(proc) ((proc->task.info.type == TYPE_THREAD)? &(proc_table[proc->task.info.ppid]):proc) 

PUBLIC int kern_get_pid();
PUBLIC PROCESS* alloc_PCB();
PUBLIC void free_PCB(PROCESS *p);
PUBLIC void sys_yield();
PUBLIC void sched_yield();
//PUBLIC void sys_sleep(int n); //deleted by mingxuan 2021-8-13
PUBLIC void sys_sleep(); //modified by mingxuan 2021-8-13
PUBLIC void wakeup(void *channel);
PUBLIC int ldt_seg_linear(PROCESS *p, int idx);
PUBLIC void* va2la(int pid, void* va);
PUBLIC void do_exit(int status);

PUBLIC void wait_for_sem(void *chan, struct spinlock *lk);
PUBLIC void wakeup_for_sem(void *chan);//modified by cjj 2021-12-23
PUBLIC void wait_event(void* event);

void restart_restore();
static inline void proc_init_context(PROCESS *p_proc) {	\
	u32 *context_base = (u32*)(proc_kstacktop(p_proc));
	p_proc->task.esp_save_context = ((char*)context_base) - PROC_CONTEXT;
	*(context_base-1) = (u32)(restart_restore);
	*(context_base-2) = 0x1202;
}
#endif