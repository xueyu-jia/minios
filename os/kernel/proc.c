
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include <kernel/type.h>
#include <kernel/const.h>
#include <kernel/clock.h>
#include <kernel/console.h>
#include <kernel/proc.h>
#include <kernel/proto.h>
#include <kernel/hd.h>
#include <kernel/string.h>
#include <kernel/buffer.h>
#include <kernel/mmap.h>
#include <kernel/pagetable.h>
#include <kernel/x86.h>

u32            cr3_ready;
int            u_proc_sum;
int            kernel_initial;
PROCESS*       p_proc_current;
PROCESS*       p_proc_next;
PUBLIC PROCESS cpu_table[NR_CPUS];
PUBLIC PROCESS proc_table[NR_PCBS];
PUBLIC TASK    task_table[NR_TASKS] = {
    {idle,          0, RPL_KRNL, 0, PROC_NICE_MAX, STACK_SIZE_TASK, "task_idle" },
    {task_tty,      1, RPL_TASK, 1, 1,             STACK_SIZE_TASK, "task_tty"  },
    {hd_service,    1, RPL_TASK, 1, 2,             STACK_SIZE_TASK, "hd_service"},
    // {bsync_service, 1, RPL_TASK, 1, 3,             STACK_SIZE_TASK, "bsync"     }
};


// // todo：增加锁
// PUBLIC PROCESS* alloc_PCB()
// {
//     PROCESS* p = proc_table + NR_K_PCBS; // 跳过前NR_K_PCBS个
//     for (int i = NR_K_PCBS; i < NR_PCBS; i++) {
//         if (p->task.stat == FREE){
//             p->task.stat = SLEEPING;
//             return p;
//         }
//         p++;
//     }
//     return NULL;
// }

// PUBLIC PROCESS* alloc_task_PCB()
// {
//     PROCESS* p = proc_table;
//     for (int i = 0; i < NR_K_PCBS; i++) {
//         if (p->task.stat == FREE){
//             p->task.stat = SLEEPING;
//             return p;
//         }
//         p++;
//     }
//     return NULL;
// }


// PUBLIC void free_PCB(PROCESS* p)
// {
//     p->task.stat = FREE;
// }

PUBLIC int kern_get_pid()
{
    return p_proc_current->task.pid;
}

PUBLIC int do_get_pid()
{
    return kern_get_pid();
}

PUBLIC int sys_get_pid()
{
    return do_get_pid();
}

PUBLIC int kern_get_pid_byname(char* name)
{
    for (PROCESS* proc = proc_table; proc < proc_table + NR_PCBS; proc++) {
        if (strcmp(proc->task.p_name, name) == 0) {
            return proc->task.pid;
        }
    }
    return -1;
}

PUBLIC int do_get_pid_byname(char* name)
{
    return kern_get_pid_byname(name);
}

PUBLIC int sys_get_pid_byname()
{
    return do_get_pid_byname((char*)get_arg(1));
}


PUBLIC void kern_get_proc_msg(proc_msg* msg)
{
    msg->pid         = p_proc_current->task.pid;
    msg->nice        = p_proc_current->task.nice;
    msg->sum_cpu_use = p_proc_current->task.sum_cpu_use;
    msg->vruntime    = p_proc_current->task.vruntime;
}

PUBLIC void do_get_proc_msg(proc_msg* msg)
{
    kern_get_proc_msg(msg);
}
PUBLIC void sys_get_proc_msg()
{
    do_get_proc_msg((proc_msg*)get_arg(1));
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void wait_for_sem(void* chan, struct spinlock* lk) {
    if (0 == p_proc_current)
        // printf("p_proc_current is unknow!");
        return;
    if (0 == lk)
        // printf("lk is unknow!");
        return;
    // Must acquire ptable.lock in order to
    // change p->state and then call sched.
    // Once we hold ptable.lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup runs with ptable.lock locked),
    // so it's okay to release lk.

    // Go to sleep.
    disable_int();
    p_proc_current->task.channel = chan;
    p_proc_current->task.stat    = SLEEPING;
    out_rq(p_proc_current);
    release(lk);
    enable_int();

    sched_yield();

    acquire(lk);
}

/*
    added by cjjx 2021-12-25
*/
void wakeup_for_sem(void* chan) {
    wakeup(chan);
}

// added by zcr
PUBLIC int ldt_seg_linear(PROCESS* p, int idx) {
    struct s_descriptor* d = &p->task.context.ldts[idx];
    return d->base_high << 24 | d->base_mid << 16 | d->base_low;
}

PUBLIC void* va2la(int pid, void* va) {
    if (kernel_initial == 1) { return va; }

    PROCESS* p        = &proc_table[pid];
    u32      seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
    u32      la       = seg_base + (u32)va;

    return (void*)la;
}

/**
 * @brief 将进程状态置为睡眠，直至事件发生
 * added by zhenhao 2023.5.19
 * @param event:等待的事件
 * @return void
 */
PUBLIC void wait_event(void* event) {
    disable_int();
    p_proc_current->task.channel = event;
    p_proc_current->task.stat    = SLEEPING;

    out_rq(p_proc_current);
    enable_int();
    sched_yield();
}

PUBLIC void idle() {
    while (1) {
        // disp_str("-idle-"); //mark debug
        out_rq(p_proc_current);
		asm volatile ("sti\n hlt" :::"memory");
    }
    // p_proc_current->task.channel = 0;
}

PRIVATE void stack_backtrace(u32 ebp) {
    disp_str("=>");
    disp_int(ebp);
    disp_str("(");
    disp_int(*(((u32*)ebp) + 1));
    disp_str(")\n");
}

PUBLIC void proc_backtrace() {
    u32 ebp;
    int skip = 2;
    disp_str("pid:");
    disp_int(proc2pid(p_proc_current));
    disp_str("\n");
    asm volatile("mov %%ebp, %0 " : "=r"(ebp));
    for (int i = 0; ebp && i < 8; i++) {
        if(i >= skip) {
            stack_backtrace(ebp);
        }
        ebp = *((u32*)ebp);
        if(ebp >= KernelLinMapBase || ebp < ShareLinLimitMAX)break;
    }
    while (1)
        ;
}

// PUBLIC PROCESS* alloc_task_PCB()
// {
// 	PROCESS* p;
// 	// p = proc_table+NR_K_PCBS;//跳过前NR_K_PCBS个
// 	p = proc_table;
// 	for(int i = 0; i < NR_K_PCBS+1; i++) {
// 		if(p->task.stat == FREE) return p;
// 		p++;
// 	}
// 	return NULL;
// }

// PUBLIC PROCESS* alloc_init_PCB()
// {
// 	PROCESS* p;
// 	// p = proc_table+NR_K_PCBS;//跳过前NR_K_PCBS个
// 	p = &proc_table[PID_INIT];
// 	if(p->task.stat == FREE)    return p;

//     disp_str("\nalloc init PCB failed\n");
//     while(1){};
// 	return NULL;
// }

// // [start,end) 地址段分配页表，如果不存在分配物理页并填写页表，否则忽略 固定地址start<end
// PRIVATE void init_proc_page_addr(u32 low, u32 high, PROCESS* proc){
// 	u32 addr;
// 	kern_mmap(proc, NULL, low, high - low, PROT_READ|PROT_WRITE, MAP_PRIVATE, 0);
// }

// // 调用此函数分配页表
// PRIVATE void init_proc_pages(PROCESS* p_proc){
// 	int pid = p_proc->task.pid;
// 	if (0 != init_proc_page(pid))
// 	{
// 		disp_color_str("kernel_main Error:init_proc_page", 0x74);
// 		return ;
// 	}
// 	p_proc->task.memmap.heap_lin_base = HeapLinBase;
// 	p_proc->task.memmap.heap_lin_limit = HeapLinBase; //堆的界限将会一直动态变化

// 	p_proc->task.memmap.stack_child_limit = StackLinLimitMAX; //add by visual 2016.5.27
// 	p_proc->task.memmap.stack_lin_base = StackLinBase;
// 	p_proc->task.memmap.stack_lin_limit = StackLinBase - 0x4000; //栈的界限将会一直动态变化，目前赋值为16k，这个值会根据esp的位置进行调整，目前初始化为16K大小
// 	p_proc->task.memmap.arg_lin_base = ArgLinBase;						 //参数内存基址
// 	p_proc->task.memmap.arg_lin_limit = ArgLinLimitMAX;
// 	p_proc->task.memmap.kernel_lin_base = KernelLinBase;
// 	p_proc->task.memmap.kernel_lin_limit = KernelLinBase + kernel_size;
// 	list_init(&p_proc->task.memmap.vma_map);
// 	init_mem_page(&p_proc->task.memmap.anon_pages, MEMPAGE_AUTO);
// 	init_proc_page_addr(p_proc->task.memmap.stack_lin_limit, p_proc->task.memmap.stack_lin_base, p_proc);
// 	init_proc_page_addr(p_proc->task.memmap.arg_lin_base, p_proc->task.memmap.arg_lin_limit, p_proc);
// 	// init_proc_page_addr(p_proc->task.memmap.stack_lin_limit, StackLinBase, pid ,PG_P | PG_USU | PG_RWW);
// 	// init_proc_page_addr(p_proc->task.memmap.arg_lin_base, p_proc->task.memmap.arg_lin_limit, pid ,PG_P | PG_USU | PG_RWW);
// }

// int kthread_create(char *name, void *func, int is_rt, int priority, int rtpriority_or_nice)
// {
// 	//禁用中断
//     PROCESS *p_proc;
//     if(strcmp(name, "initial") == 0)    p_proc = alloc_init_PCB();
//     else p_proc = alloc_task_PCB();

// 	if(p_proc == NULL){
// 		disp_color_str("kthread_create: alloc PCB error\n", 0x74);
// 		return -1;
// 	}

// 	// // kernel线程共用一个mmu，相同的内核地址空间，共用一套内核页表。  存在问题
// 	// p_proc->task.mmu = kernel_mmu; // use kernel vmap
// 	// init_kernel_mmu(p_proc->task.pid, &p_proc->task.mmu);
//     init_proc_pages(p_proc);

// 	// kernel线程共用一个ldt
// 	// p_proc->task.context.ldt_sel = alloc_kern_ldt();
// 	// alloc_ldt(true, p_proc-proc_table, &p_proc->task.context);

// 	init_cpu_context(&p_proc->task.context, p_proc->task.pid, func, StackLinBase, restart_restore, priority);

// 	// pid不应该和PCB的位置绑定，需要一个单独的alloc_pid() mark lirong
// 	init_process(p_proc, name, READY, p_proc->task.pid, is_rt, rtpriority_or_nice);

// 	// 进程树初始化
// 	p_proc->task.tree_info.type = TYPE_PROCESS; 		//当前是进程还是线程
// 	p_proc->task.tree_info.real_ppid = -1;	   		//亲父进程，创建它的那个进程
// 	p_proc->task.tree_info.ppid = -1;		   		//当前父进程
// 	p_proc->task.tree_info.child_p_num = 0;	   		//子进程数量
// 	//p_proc->task.info.child_process[NR_CHILD_MAX];//子进程列表
// 	p_proc->task.tree_info.child_t_num = 0; 			//子线程数量
// 	//p_proc->task.info.child_thread[NR_CHILD_MAX];//子线程列表
// 	p_proc->task.tree_info.text_hold = 1; 			//是否拥有代码
// 	p_proc->task.tree_info.data_hold = 1; 			//是否拥有数据

// 	//恢复中断
// 	//intr_restore(intr);
// 	return p_proc->task.pid;
// }

// /*************************************************************************
// 进程基本信息初始化（name、stat等）
// added by lcy, 2023.10.22
// ***************************************************************************/
// PUBLIC void init_process(PROCESS *proc, char name[32], enum proc_stat stat, int pid, int is_rt, int priority_or_nice){
// 		strcpy(proc->task.p_name, name); //名称
// 		proc->task.pid = pid;					   //pid
// 		proc->task.stat = stat;				   //初始化状态 -1表示未初始化
// 		proc->task.ticks = proc->task.priority = 1;	//时间片和优先级
// 		//proc->task.regs.eip = eip;

// 		//CFS
// 		proc->task.is_rt=is_rt;
// 		if(is_rt) {
// 			proc->task.rt_priority=priority_or_nice;
// 		}else {
// 			proc->task.nice=priority_or_nice;
// 			proc->task.weight=nice_to_weight[proc->task.nice+20];
// 		}
// 		proc->task.vruntime=0;
// 		proc->task.sum_cpu_use=0;
// 		proc->task.cpu_use=0;

// }

// void init_all_PCB()
// {
// 	if(kernel_initial != 1)		return;
// 	// init_kernel_mm();

// 	PROCESS *p = proc_table;
// 	// for(int i = 0; i < NR_PCBS; i++, p++){
//     for(int i = 0; i < NR_K_PCBS+1; i++, p++){

// 		memset(p, 0, sizeof(PROCESS));

// 		// p->task.stat = -1; 			//初始化状态 -1表示未初始化
// 		// p->task.ticks = -1;
// 		// p->task.priority = -1;
// 		p->task.pid = i;
// 		p->task.p_name[0] = (char)0;
// 		p->task.stat = FREE;
// 		// // p->task.cwd[0] = (char)0;
// 		// p->task.suspended = -1;
// 		// p->task.exit_status = -1;
// 		// p->task.child_exit_status = -1;
// 		// p->task.sig_set = 0;
// 		// p->task._Hanlder = NULL;
// 		// p->task.retval = NULL;
// 		// p->task.who_wait_flag = 0;

// 		// 开辟上下文帧的空间
// 		char *frame = (char *)(p + 1);
// 		frame -= sizeof(STACK_FRAME);
// 		p->task.context.esp_save_int = (STACK_FRAME*)frame;
// 		// 开辟中断帧的空间
// 		frame -= sizeof(CONTEXT_FRAME);
// 		p->task.context.esp_save_context = (CONTEXT_FRAME*)frame;

// 		// init_cpu_context(&p->task.context, i, NULL, StackLinBase, (u32)restart_restore, PRIVILEGE_USER);
//         init_user_cpu_context(&p->task.context, i);

// 		for (int j = 0; j < NR_FILES; j++){
// 			proc_table[i].task.filp[j] = 0;
// 		}
// 		// 暂时不需要初始化mmu
// 	}
// }

// void init_a_PCB(PROCESS* pcb)
// {
//     memset(pcb, 0, sizeof(PROCESS));
//     // pcb->task.stat = -1; 			//初始化状态 -1表示未初始化
//     // pcb->task.ticks = -1;
//     // pcb->task.priority = -1;
//     pcb->task.pid = pcb - proc_table;
//     pcb->task.p_name[0] = (char)0;
//     pcb->task.stat = FREE;
//     // p->task.cwd[0] = (char)0;
//     // pcb->task.suspended = -1;
//     // pcb->task.exit_status = -1;
//     // pcb->task.child_exit_status = -1;
//     // pcb->task.sig_set = 0;
//     // pcb->task._Hanlder = NULL;
//     // pcb->task.retval = NULL;
//     // pcb->task.who_wait_flag = 0;

//     // 开辟上下文帧的空间
//     char *frame = (char *)(pcb + 1);
//     frame -= sizeof(STACK_FRAME);
//     pcb->task.context.esp_save_int = (STACK_FRAME*)frame;
//     // 开辟中断帧的空间
//     frame -= sizeof(CONTEXT_FRAME);
//     pcb->task.context.esp_save_context = (CONTEXT_FRAME*)frame;
//     init_user_cpu_context(&pcb->task.context, pcb->task.pid);
//     for (int j = 0; j < NR_FILES; j++){
//         pcb->task.filp[j] = 0;
//     }
// }

// void set_rpl(cpu_context *context, int pid, u32 rpl)
// {
//     init_cpu_context(context, pid,
//     context->esp_save_int->eip,
//     context->esp_save_int->esp,
//     (u32)restart_restore,
//     rpl);
// }

// void init_user_cpu_context(cpu_context *context, int pid)
// {
//     // 开辟上下文帧的空间
//     PROCESS* pcb = pid2proc(pid);
//     // char *frame = (char *)(pcb + 1);
//     // frame -= sizeof(STACK_FRAME);
//     // pcb->task.context.esp_save_int = (STACK_FRAME*)frame;
//     pcb->task.context.esp_save_int = proc_stack_frame(pcb);
//     // 开辟中断帧的空间
//     // frame -= sizeof(CONTEXT_FRAME);
//     // pcb->task.context.esp_save_context = (CONTEXT_FRAME*)frame;
//     pcb->task.context.esp_save_context = proc_context_frame(pcb);

//     set_rpl(context, pid, RPL_USER);
// }
