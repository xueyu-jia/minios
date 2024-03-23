
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "hd.h"
#include "buffer.h"

u32 cr3_ready;
int     u_proc_sum;
PROCESS*	p_proc_current;
PROCESS*	p_proc_next;
PUBLIC	PROCESS			cpu_table[NR_CPUS];
PUBLIC	PROCESS			proc_table[NR_PCBS];
PUBLIC	TASK	task_table[NR_TASKS] = {{hd_service, STACK_SIZE_TASK, "hd_service"},
										{task_tty, STACK_SIZE_TASK, "task_tty"},
										{bsync_service, STACK_SIZE_TASK, "bsync"}};
/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS* p;
	int	 greatest_ticks = 0;

	//Added by xw, 18/4/21
	if (p_proc_current->task.stat == READY && p_proc_current->task.ticks > 0) {
		p_proc_next = p_proc_current;	//added by xw, 18/4/26
		return;
	}

	while (!greatest_ticks)
	{
		for (p = proc_table; p < proc_table+NR_PCBS; p++)		//edit by visual 2016.4.5
		{
			if (p->task.stat == READY && p->task.ticks > greatest_ticks)  //edit by visual 2016.4.5
			{
				greatest_ticks = p->task.ticks;
				// p_proc_current = p;
				p_proc_next	= p;	//modified by xw, 18/4/26
			}

		}

		if (!greatest_ticks)
		{
			for (p = proc_table; p < proc_table+NR_PCBS; p++) //edit by visual 2016.4.5
			{
				p->task.ticks = p->task.priority;
			}
		}
	}
	cr3_ready = p_proc_next->task.cr3;
}

/*======================================================================*
                           alloc_PCB  add by visual 2016.4.8
 *======================================================================*/
PUBLIC PROCESS* alloc_PCB()
{//分配PCB表
	PROCESS* p;
	int i;
	p=proc_table+NR_K_PCBS;//跳过前NR_K_PCBS个
	for(i=NR_K_PCBS;i<NR_PCBS;i++)
	{
		// if(p->task.stat==IDLE)break;
		if(p->task.stat==FREE)break; //FREE表示当前PCB是空闲的, modified by mingxuan 2021-8-21
		p++;
	}
	if(i>=NR_PCBS)	return 0;   //NULL
	else	return p;
}

/*======================================================================*
                           free_PCB  add by visual 2016.4.8
 *======================================================================*/
PUBLIC void free_PCB(PROCESS *p)
{	//释放PCB表
	// p->task.stat=IDLE;
	p->task.stat=FREE; //FREE表示当前PCB是空闲的, modified by mingxuan 2021-8-21
}

/*======================================================================*
                           get_pid		add by visual 2016.4.6
 *======================================================================*/
PUBLIC int sys_get_pid()
{
	return do_get_pid();
}

PUBLIC int do_get_pid()
{
	return kern_get_pid();
}

//added by mingxuan 2021-8-14
PUBLIC int kern_get_pid()
{
	return p_proc_current->task.pid;
}

PUBLIC int sys_get_pid_byname()
{
	return do_get_pid_byname(get_arg(1));
}

PUBLIC int do_get_pid_byname(char *name)
{
	return kern_get_pid_byname(name);
}

PUBLIC int kern_get_pid_byname(char *name)
{
	for(PROCESS *proc = proc_table; proc < proc_table+NR_PCBS;proc++)
	{
		if(strcmp(proc->task.p_name, name)==0)
		{
			return proc->task.pid;
		}
	}
	return -1;
}
/*======================================================================*
                           yield and sleep
 *======================================================================*/
//used for processes to give up the CPU
PUBLIC void kern_yield()
{
	p_proc_current->task.ticks = 0;	/* modified by xw, 18/4/27 */
	sched();	//Modified by xw, 18/4/19
}

PUBLIC void do_yield()
{
	kern_yield();
}

PUBLIC void sys_yield()
{
	do_yield();
}

PUBLIC void kern_sleep(int n)
{
	int ticks0;

	ticks0 = ticks;
	p_proc_current->task.channel = &ticks;

	while(ticks - ticks0 < n){
		p_proc_current->task.stat = SLEEPING;
		sched();	//Modified by xw, 18/4/19
	}
	p_proc_current->task.channel = 0;
}

PUBLIC void do_sleep(int n)
{
	return kern_sleep(n);
}

PUBLIC void sys_sleep()
{
    return do_sleep(get_arg(1));
}

/*invoked by clock-interrupt handler to wakeup
 *processes sleeping on ticks.
 */
PUBLIC void sys_wakeup(void *channel)
{
	PROCESS *p;

	for(p = proc_table; p < proc_table + NR_PCBS; p++){
		if(p->task.stat == SLEEPING && p->task.channel == channel){
			p->task.stat = READY;
		}
	}
}
/*
	added by cjjx 2021-12-25
*/
// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void wait_for_sem(void *chan, struct spinlock *lk)
{
	if(0 == p_proc_current)
		// printf("p_proc_current is unknow!");
		return 0; 
	if(0 == lk)
		// printf("lk is unknow!");
		return 0; 
	// Must acquire ptable.lock in order to
	// change p->state and then call sched.
	// Once we hold ptable.lock, we can be
	// guaranteed that we won't miss any wakeup
	// (wakeup runs with ptable.lock locked),
	// so it's okay to release lk.

	// Go to sleep.
	p_proc_current->task.channel = chan;
	p_proc_current->task.stat = SLEEPING;
	release(lk);
	
	u32 ringlevel = get_ring_level();
	if (ringlevel == 0) {
		sched();
	} else {
		yield();
	}
	
	acquire(lk);
	// Tidy up.
	p_proc_current->task.channel = 0;

}

/*
	added by cjjx 2021-12-25
*/
void wakeup_for_sem(void *chan)
{

	sys_wakeup(chan);

}


//added by zcr
PUBLIC int ldt_seg_linear(PROCESS *p, int idx)
{
	struct s_descriptor * d = &p->task.ldts[idx];
	return d->base_high << 24 | d->base_mid << 16 | d->base_low;
}

PUBLIC void* va2la(int pid, void* va)
{
	if(kernel_initial == 1){
		return va;
	}

	PROCESS* p = &proc_table[pid];
	u32 seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
	u32 la = seg_base + (u32)va;

	return (void*)la;
}
//~zcr

/**
 * @brief 将进程状态置为睡眠，直至事件发生
 * added by zhenhao 2023.5.19
 * @param event:等待的事件 
 * @return void 
 */
PUBLIC void wait_event(void* event) {
	p_proc_current->task.channel = event;
	p_proc_current->task.stat = SLEEPING;
	
	u32 ringlevel = get_ring_level();
  	if (ringlevel == 0) {
  		sched();	//非系统调用（sched中有为cr3赋值的操作，只有ring0级别才能执行）
  	} else {
		yield();	//系统调用
  	}
	p_proc_current->task.channel = 0;
}