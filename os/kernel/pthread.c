/******************************************************************
*			pthread.c //add by visual 2016.5.26
*系统调用pthread()
*******************************************************************/

#include <kernel/type.h>
#include <kernel/const.h>
#include <kernel/proc.h>
#include <kernel/fs.h>
#include <kernel/pthread.h>
#include <kernel/memman.h>
#include <kernel/proto.h>

PRIVATE int pthread_pcb_cpy(PROCESS *p_child,PROCESS *p_parent);
PRIVATE int pthread_update_info(PROCESS *p_child,PROCESS *p_parent);
PRIVATE int pthread_stack_init(PROCESS *p_child,PROCESS *p_parent, pthread_attr_t *attr);
PRIVATE int pthread_heap_init(PROCESS *p_child,PROCESS *p_parent);

/**********************************************************
*		sys_pthread			//add by visual 2016.5.25
*系统调用sys_pthread的具体实现部分
*************************************************************/
//PUBLIC int sys_pthread(void *entry)
//PUBLIC int do_pthread(void *entry)	//modified by mingxuan 2021-8-11
PUBLIC int kern_pthread_create(pthread_t *thread, pthread_attr_t *attr, void *entry, void *arg)	//modified by mingxuan 2021-8-14
{
	PROCESS* p_child;

	/*if(p_proc_current->task.tree_info.type == TYPE_THREAD )
	{//线程不能创建线程
		disp_color_str("[pthread failed:",0x74);
		disp_color_str(p_proc_current->task.p_name,0x74);
		disp_color_str("]",0x74);
		return -1;
	}*/

	PROCESS *p_parent;
	// if( p_proc_current->task.tree_info.type == TYPE_THREAD )
	// {//线程
	// 	p_parent = &(proc_table[p_proc_current->task.tree_info.ppid]);//父进程
	// }
	// else
	// {//进程
	// 	p_parent = p_proc_current;//父进程就是父线程
	// }
	p_parent = proc_real(p_proc_current);
	/*****************如若attr为null，新建一默认attr**********************/

	pthread_attr_t true_attr;
	if(attr == NULL)
	{
		//true_attr.detachstate		= PTHREAD_CREATE_DETACHED;
		true_attr.detachstate		= PTHREAD_CREATE_JOINABLE;//modified by dongzhangqi 2023.5.4 Detachstate的默认设置应为joinable，允许调用pthread_join
		true_attr.schedpolicy		= SCHED_FIFO;
		true_attr.inheritsched		= PTHREAD_INHERIT_SCHED;
		true_attr.scope			= PTHREAD_SCOPE_PROCESS;
		true_attr.guardsize		= 0;
		true_attr.stackaddr_set		= 0;
		true_attr.stackaddr		= p_parent->task.memmap.stack_child_limit + 0x4000;
		true_attr.stacksize		= 0x4000;
	}
	else
	{
		true_attr = *attr;
	}



	/*****************申请空白PCB表**********************/
	p_child = alloc_PCB();
	if( 0==p_child )
	{
		disp_color_str("PCB NULL,pthread faild!",0x74);
		return -1;
	}
	else
	{
		lock_or_yield(&p_parent->task.lock);
		/************复制父进程的PCB部分内容（保留了自己的标识信息,但cr3使用的是父进程的）**************/
		pthread_pcb_cpy(p_child, p_parent);
		release(&p_child->task.lock);
		/**************更新进程树标识info信息************************/
		pthread_update_info(p_child,p_parent); //此步骤先完成，mmap由此直接找到父进程的memmap jiangfeng 2024.05
		/************在父进程的栈中分配子线程的栈（从进程栈的低地址分配8M,注意方向）**********************/
		pthread_stack_init(p_child, p_parent, &true_attr);

		/**************初始化子线程的堆（此时的这两个变量已经变成了指针）***********************/
		pthread_heap_init(p_child, p_parent);

		/********************设置线程的执行入口**********************************************/
		//p_child->task.regs.eip = (u32)entry;
		STACK_FRAME* p_reg = proc_kstacktop(p_child);
		// p_reg = (char*)(p_child + 1);	//added by xw, 17/12/11
		//*((u32*)(p_reg + EIPREG - P_STACKTOP)) = p_child->task.regs.eip;	//added by xw, 17/12/11
		// *((u32*)(p_reg + EIPREG - P_STACKTOP)) = (u32)entry;
		p_reg->eip = entry;
		p_reg->esp = p_child->task.memmap.stack_lin_base;

		/**************修改子线程的线程属性************************/
		p_child->task.attr = true_attr;    //added by dongzhangqi 2023.5.7
		p_child->task.who_wait_flag = -1;

		/************修改子进程的名字***************/
		strcpy(p_child->task.p_name,"pthread");	// 所有的子进程都叫pthread

		/*************子进程返回值在其eax寄存器***************/
		//p_child->task.regs.eax = 0;//return child with 0
		// p_reg = (char*)(p_child + 1);	//added by xw, 17/12/11
		//*((u32*)(p_reg + EAXREG - P_STACKTOP)) = p_child->task.regs.eax;	//added by xw, 17/12/11
		// *((u32*)(p_reg + EAXREG - P_STACKTOP)) = 0;

		p_reg->eax = 0;
		/*************子进程参数***************/
		/*
		p_child->task.regs.esp -= num_4B;
		*((u32*)(p_child->task.regs.esp)) = (u32*)arg;
		p_child->task.regs.esp -= num_4B;
		*((u32*)(p_reg + ESPREG - P_STACKTOP)) = p_child->task.regs.esp;*/ //deleted by lcy 2023.10.25
		// u32 reg_esp= *((u32*)(p_reg + ESPREG - P_STACKTOP));
		u32 reg_esp = p_reg->esp;
		reg_esp -= num_4B;
		*((u32 *)reg_esp) = (u32*)arg;
		// *((u32*)(p_reg + ESPREG - P_STACKTOP)) -= num_4B*2;
		p_reg->esp -= num_4B*2;


		/****************用户进程数+1****************************/
		u_proc_sum += 1;

		disp_color_str("[pthread success:",0x72);
		disp_color_str(p_proc_current->task.p_name,0x72);
		disp_color_str("]",0x72);

		//anything child need is prepared now, set its state to ready. added by xw, 17/12/11
		p_child->task.stat = READY;
		p_child->task.pthread_id=p_parent->task.tree_info.child_t_num+1;//Add By ZengHao & MaLinhan 21.12.22
		in_rq(p_child);
		release(&p_child->task.lock);
		release(&p_parent->task.lock);
	}
	*thread = p_child->task.pid;
	return 0;
}

//added by mingxuan 2021-8-14
PUBLIC int do_pthread_create(int *thread, void *attr, void *entry, void *arg)
{
	return kern_pthread_create(thread, attr, entry, arg);
}

//added by mingxuan 2021-8-11
PUBLIC int sys_pthread_create()
{
    return do_pthread_create(get_arg(1), get_arg(2), get_arg(3), get_arg(4));
}

/**********************************************************
*		pthread_pcb_cpy			//add by visual 2016.5.26
*复制父进程PCB表，但是又马上恢复了子进程的标识信息
*************************************************************/
PRIVATE int pthread_pcb_cpy(PROCESS *p_child,PROCESS *p_parent)
{
	int pid;
	u32 eflags, cr3_child;
	char* p_reg;	//point to a register in the new kernel stack, added by xw, 17/12/11
	CONTEXT_FRAME* esp_save_context;	//use to save corresponding field in child's PCB, xw, 18/4/21
	STACK_FRAME* esp_save_int;  		//added by lcy, 2023.10.24
	//暂存标识信息
	pid = p_child->task.pid;

	//复制PCB内容
	//esp_save_int and esp_save_context must be saved, because the child and the parent
	//use different kernel stack! And these two are importent to the child's initial running.
	//Added by xw, 18/4/21
	esp_save_int = p_child->task.context.esp_save_int;
	esp_save_context = p_child->task.context.esp_save_context;
	disable_int();
	p_child->task = p_parent->task;
	//note that syscalls can be interrupted now! the state of child can only be setted
	//READY when anything else is well prepared. if an interruption happens right here,
	//an error will still occur.
	// fixed jiangfeng , disable int to protect 2024.4
	p_child->task.stat = SLEEPING; //统一PCB state jiangfeng 20240314
	enable_int();
	init_mem_page(&p_child->task.memmap.anon_pages, MEMPAGE_AUTO); //线程使用父进程的mmap 链表，此两项直接重置为空 jiangfeng 202405
	list_init(&p_child->task.memmap.vma_map);
	// p_child->task.esp_save_context = esp_save_context;	//same above
	memcpy((char*) proc_kstacktop(p_child), proc_kstacktop(p_parent), P_STACKTOP);//changed by lcy 2023.10.26 19*4
	//modified end
	p_child->task.pid = pid;
	init_user_cpu_context(&p_child->task.context, p_child->task.pid);
	// proc_init_ldt_kstack(p_child, RPL_USER);
	// proc_init_context(p_child);
	//恢复标识信息


	//p_child->task.regs.eflags = eflags;
	// p_reg = (char*)(p_child + 1);	//added by xw, 17/12/11
	// *((u32*)(p_reg + EFLAGSREG - P_STACKTOP)) = eflags;	//added by xw, 17/12/11
	return 0;
}



/**********************************************************
*		pthread_update_info			//add by visual 2016.5.26
*更新父进程和子线程程的进程树标识info
*************************************************************/
PRIVATE int pthread_update_info(PROCESS* p_child,PROCESS *p_parent)
{
	/************更新父进程的info***************///注意 父进程 父进程 父进程
	if( p_parent!=p_proc_current )
	{//只有在线程创建线程的时候才会执行	，p_parent事实上是父进程
		p_parent->task.tree_info.child_t_num += 1;	//子线程数量
		p_parent->task.tree_info.child_thread[p_parent->task.tree_info.child_t_num-1] = p_child->task.pid;//子线程列表
	}
	/************更新父线程的info**************/
	//p_proc_current->task.tree_info.type;		//当前是进程还是线程
	//p_proc_current->task.tree_info.real_ppid;  //亲父进程，创建它的那个进程
	//p_proc_current->task.tree_info.ppid;		//当前父进程
	//p_proc_current->task.tree_info.child_p_num += 1; //子进程数量
	//p_proc_current->task.tree_info.child_process[p_proc_current->task.tree_info.child_p_num-1] = p_child->task.pid;//子进程列表
	p_proc_current->task.tree_info.child_t_num += 1;	//子线程数量
	p_proc_current->task.tree_info.child_thread[p_proc_current->task.tree_info.child_t_num-1] = p_child->task.pid;//子线程列表
	//p_proc_current->task.text_hold;			//是否拥有代码
	//p_proc_current->task.data_hold;			//是否拥有数据

	/************更新子线程的info***************/
	p_child->task.tree_info.type = TYPE_THREAD ;//这是一个线程
	p_child->task.tree_info.real_ppid = p_proc_current->task.pid;  //亲父进程，创建它的那个线程，注意，这个是创建它的那个线程p_proc_current
	p_child->task.tree_info.ppid = p_parent->task.pid;		//当前父进程
	p_child->task.tree_info.child_p_num = 0; //子进程数量
	//p_child->task.tree_info.child_process[NR_CHILD_MAX] = pid;//子进程列表
	p_child->task.tree_info.child_t_num = 0;	//子线程数量
	//p_child->task.tree_info.child_thread[NR_CHILD_MAX];//子线程列表
	p_child->task.tree_info.text_hold = 0;			//是否拥有代码，子进程不拥有代码
	p_child->task.tree_info.data_hold = 0;			//是否拥有数据，子进程拥有数据

	return 0;
}

/**********************************************************
*		pthread_stack_init			//add by visual 2016.5.26
*申请子线程的栈，并重置其esp
*************************************************************/
PRIVATE int pthread_stack_init(PROCESS* p_child,PROCESS *p_parent, pthread_attr_t *attr)
{
	int addr_lin;
	char* p_reg;	//point to a register in the new kernel stack, added by xw, 17/12/11

	p_child->task.memmap.stack_lin_limit = attr->stackaddr - attr->stacksize;//子线程的栈界
	p_parent->task.memmap.stack_child_limit += attr->stacksize; //分配16K
	p_child->task.memmap.stack_lin_base = attr->stackaddr;	//子线程的基址

	// for( addr_lin=p_child->task.memmap.stack_lin_base ; addr_lin>p_child->task.memmap.stack_lin_limit ; addr_lin-=num_4K)//申请物理地址
	// {
	// 	//disp_str("#");
	// 	//lin_mapping_phy(addr_lin, MAX_UNSIGNED_INT, p_child->task.pid, PG_P  | PG_USU | PG_RWW,PG_P  | PG_USU | PG_RWW);
	// 	ker_umalloc_4k(addr_lin,p_child->task.pid,PG_P  | PG_USU | PG_RWW);  //edited by wang 2021.8.29
	// }
	kern_mmap(p_child, NULL, p_child->task.memmap.stack_lin_limit,
		attr->stacksize, PROT_READ|PROT_WRITE, MAP_PRIVATE, 0);

	//p_child->task.regs.esp = p_child->task.memmap.stack_lin_base;		//调整esp
	// p_reg = (char*)(p_child + 1);	//added by xw, 17/12/11
	//*((u32*)(p_reg + ESPREG - P_STACKTOP)) = p_child->task.regs.esp;	//added by xw, 17/12/11
	// *((u32*)(p_reg + ESPREG - P_STACKTOP)) = p_child->task.memmap.stack_lin_base;
	proc_kstacktop(p_child)->esp = p_child->task.memmap.stack_lin_base;

	//p_child->task.regs.ebp = p_child->task.memmap.stack_lin_base;		//调整ebp
	// p_reg = (char*)(p_child + 1);	//added by xw, 17/12/11
	//*((u32*)(p_reg + EBPREG - P_STACKTOP)) = p_child->task.regs.ebp;	//added by xw, 17/12/11
	// *((u32*)(p_reg + EBPREG - P_STACKTOP)) = p_child->task.memmap.stack_lin_base;
	proc_kstacktop(p_child)->ebp = p_child->task.memmap.stack_lin_base;

	return 0;
}
/**********************************************************
*		pthread_stack_init			//add by visual 2016.5.26
*子线程使用父进程的堆
*************************************************************/
PRIVATE int pthread_heap_init(PROCESS* p_child,PROCESS *p_parent)
{
	p_child->task.memmap.heap_lin_base = (u32)&(p_parent->task.memmap.heap_lin_base);
	p_child->task.memmap.heap_lin_limit = (u32)&(p_parent->task.memmap.heap_lin_limit);
	return 0;
}

/* added by ZengHao & MaLinhan 2021.12.23  */
PUBLIC pthread_t  sys_pthread_self()
{
	return do_pthread_self();
}

PUBLIC pthread_t  do_pthread_self()
{
	return kern_pthread_self();
}

PUBLIC pthread_t  kern_pthread_self()
{
	return p_proc_current->task.pthread_id;
}
/* 				end added  				   */


/**********************************************************
*		pthread_exit			//add by dongzhangqi 2023.5.4
*
*************************************************************/



PUBLIC void kern_pthread_exit(void *retval)
{

    PROCESS *p_proc = p_proc_current;


	if(p_proc->task.who_wait_flag != -1){ //有进程正在等待自己
		PROCESS *p_father = pid2proc(p_proc->task.who_wait_flag);
		p_proc->task.who_wait_flag = -1;
		wakeup(p_father);
	}

	//设置返回值
	p_proc->task.retval = retval;
	p_proc->task.stat = ZOMBY;
	out_rq(p_proc);
	sched_yield();
	return;

}

PUBLIC void do_pthread_exit(void *retval)
{
	return kern_pthread_exit(retval);
}

PUBLIC void sys_pthread_exit()
{
    return do_pthread_exit(get_arg(1));
}



/**********************************************************
*		pthread_join			//add by dongzhangqi 2023.5.4
*
*************************************************************/

PUBLIC int kern_pthread_join(pthread_t thread, void **retval)
{
	PROCESS *p_proc_father = p_proc_current;//发起等待的进程

	if(p_proc_father->task.tree_info.child_t_num == 0)
	{
		disp_color_str("no child_pthread!! error\n", 0x74);
		return -1;
	}

	PROCESS *p_proc_child = &proc_table[thread];//要等待的目标子线程

	//子线程不是joinable的
	if(p_proc_child->task.attr.detachstate != PTHREAD_CREATE_JOINABLE){
		disp_color_str("pthread is not joinable!! error\n", 0x74);
		return -1;
	}
	while(1){
		lock_or_yield(&p_proc_father->task.lock);
		if(p_proc_child->task.stat != ZOMBY){
			//挂起，并告知被等待的线程，线程退出时再唤醒
			lock_or_yield(&p_proc_child->task.lock);
			p_proc_child->task.who_wait_flag = p_proc_father->task.pid;
			release(&p_proc_child->task.lock);
			release(&p_proc_father->task.lock);
			wait_event(p_proc_father);
		}
		else{
			lock_or_yield(&p_proc_child->task.lock);
			//获取返回值
			if(retval != NULL){
				*retval = p_proc_child->task.retval;
			}

			p_proc_father->task.tree_info.child_t_num--;

			p_proc_father->task.tree_info.child_thread[thread] = 0;

			//释放栈物理页
			// free_seg_phypage(p_proc_child->task.pid, MEMMAP_STACK);

			//释放栈页表
			// free_seg_pagetbl(p_proc_child->task.pid, MEMMAP_STACK);
			release(&p_proc_child->task.lock);
			//释放PCB
			free_PCB(p_proc_child);

			release(&p_proc_father->task.lock);
			break;
		}
	}

	return 0;

}

PUBLIC int do_pthread_join(pthread_t thread, void **retval)
{
	return kern_pthread_join(thread, retval);
}

PUBLIC int sys_pthread_join()
{
    return do_pthread_join(get_arg(1), get_arg(2));
}
