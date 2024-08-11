/*****************************************************
*			fork.c			//add by visual 2016.5.25
*系统调用fork()功能实现部分sys_fork()
********************************************************/

#include "const.h"
#include "proc.h"
#include "proto.h"
#include "string.h"
#include "fs.h"
#include "pagetable.h"
#include "memman.h"

PRIVATE int fork_pcb_cpy(PROCESS* p_child);
PRIVATE int update_proc_tree(PROCESS* p_child);


/**
 * @brief 系统调用sys_fork具体实现
 * @retval 子进程的pid
*/
PUBLIC int kern_fork()
{
	// alloc a new PCB for child proc
	PROCESS* fa = p_proc_current;
	lock_or_yield(&fa->task.lock);
	PROCESS* ch = alloc_PCB();
	if(0 == ch)
		goto alloc_PCB_faile;

	// copy information from parent PCB
	if(ch->task.cr3 == 0)	//如果没有页目录，就申请 //added by mingxuan 2021-1-11
		init_proc_page(ch->task.pid);

	fork_pcb_cpy(ch);
	release(&ch->task.lock);

	/**************复制线性内存，包括堆、栈、代码数据等等***********************/
	// mmu change: jiangfeng 2024.05
	memmap_copy(fa, ch);		
		// todo: error handler

	// update the proc tree
	update_proc_tree(ch);

	// rename the child proc
	strcpy(ch->task.p_name,"fork");	// 所有的子进程都叫fork
	
	// set the retval of child proc
	proc_kstacktop(ch)->eax = 0; // 设置返回值，改为使用内核栈宏 2024.05.05 jiangfeng
	
	u_proc_sum += 1;

	//anything child need is prepared now, set its state to ready. added by xw, 17/12/11
	ch->task.stat = READY;
	
	release(&fa->task.lock);
	in_rq(ch);
	
	return ch->task.pid;	

alloc_PCB_faile:
	disp_color_str("PCB NULL,fork faild!",0x74);
	return -1;

}

PUBLIC int do_fork()
{
	return kern_fork();
}

PUBLIC int sys_fork()
{
	return do_fork();
}


/**
 * @brief	复制父进程的PCB，但保留子进程独有的信息
 * @retval	0
 * @param	child's PROCESS
 */
PRIVATE int fork_pcb_cpy(PROCESS* p_child)
{
	int pid;
	u32 cr3_child;
	// STACK_FRAME* p_reg = proc_kstacktop(p_child);

	// fork之后的子进程此处的指针不能使用父进程pcb中的上下文指针，
	// 也不应该使用旧的无效pcb中的上下文，简单起见这里直接调用初始化重置上下文字段
	// jiangfeng  2024.5
	STACK_FRAME *esp_save_stackframe;				//added by lcy, 2023.10.24
	//暂存子进程的标识信息
	pid = p_child->task.pid;
	cr3_child = p_child->task.cr3; 
	esp_save_stackframe = p_child->task.esp_save_int;

	// 复制父进程的PCB
	disable_int();
	p_child->task = p_proc_current->task;
	//note that syscalls can be interrupted now! the state of child can only be setted
	//READY when anything else is well prepared. if an interruption happens right here,
	//an error will still occur.
	p_child->task.stat = SLEEPING;
	enable_int();

	//恢复子进程PCB独有的信息
	p_child->task.cpu_use = 0;
	p_child->task.sum_cpu_use = 0;
	p_child->task.esp_save_int = esp_save_stackframe;	//esp_save_int of child must be restored!!

	memcpy((char*) proc_kstacktop(p_child), proc_kstacktop(p_proc_current), P_STACKTOP);//changed by lcy 2023.10.26 19*4
	p_child->task.pid = pid;
	
	proc_init_ldt_kstack(p_child, RPL_USER); // 改为统一的宏函数初始化 jiangfeng 2024.05
	proc_init_context(p_child);		
	p_child->task.cr3 = cr3_child;

	atomic_inc(&p_child->task.cwd->d_count); // 子进程增加父进程打开文件的引用计数 jiangfeng 2024.02
	for(int i=0; i<NR_FILES; i++){
		if(p_child->task.filp[i]){
			fget(p_child->task.filp[i]);
		}
	}
	return 0;
}



/**
 * @brief	更新父进程和子进程的进程树信息
 * @param	child's PROCESS
 * @retval 	0
 */

PRIVATE int update_proc_tree(PROCESS* p_child)
{
	/************更新父进程的info***************/		
	p_proc_current->task.info.child_p_num += 1; //子进程数量
	p_proc_current->task.info.child_process[p_proc_current->task.info.child_p_num-1] = p_child->task.pid;//子进程列表
		
	/************更新子进程的info***************/	
	p_child->task.info.type = p_proc_current->task.info.type;	//当前进程属性跟父进程一样
	p_child->task.info.real_ppid = p_proc_current->task.pid;  //亲父进程，创建它的那个进程
	p_child->task.info.ppid = p_proc_current->task.pid;		//当前父进程	
	p_child->task.info.child_p_num = 0; //子进程数量
	p_child->task.info.child_t_num = 0;	//子线程数量
	p_child->task.info.text_hold = 0;			//是否拥有代码，子进程不拥有代码
	p_child->task.info.data_hold = 1;			//是否拥有数据，子进程拥有数据
	
	return 0;
}
