/*
#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "fs.h"
#include "fs_misc.h"
#include "spinlock.h"
*/
#include "../include/type.h"
#include "../include/const.h"
#include "../include/protect.h"
#include "../include/string.h"
#include "../include/proc.h"
#include "../include/global.h"
#include "../include/proto.h"
#include "../include/fs.h"
#include "../include/fs_misc.h"
#include "../include/spinlock.h"

PUBLIC int sys_wait(void) //wait返回的为子进程退出的状态即status
{
	return do_wait();
}

PUBLIC int do_wait(void) //wait返回的为子进程退出的状态即status
{
	return kern_wait();
}

/*======================================================================*
                      sys_wait            added by mingxuan 2021-1-6
 *======================================================================*/
//PUBLIC int sys_wait(void) //wait返回的为子进程退出的状态即status
PUBLIC int kern_wait(void) //wait返回的为子进程退出的状态即status //modified by mingxuan 2021-8-21
{
    u32 exit_child_hanging = 0;
	//看有无子进程，如果没有子进程就打印错误，并返回-1
	if(p_proc_current->task.info.child_p_num == 0)
	{
		disp_str("no child_process!! error\n");
		return -1;
	}

    //遍历子进程看有无正ZOMBY的
	while(!exit_child_hanging)
	{
        int i;
		for(i = 0;i < NR_CHILD_MAX;i++)
		{
			//该子进程表项中有值且处于ZOMBY
			if(p_proc_current->task.info.child_process[i] !=0 && proc_table[p_proc_current->task.info.child_process[i]].task.we_flag == ZOMBY)
			{
				exit_child_hanging = 1;

				//置子进程的状态为正常
				proc_table[p_proc_current->task.info.child_process[i]].task.we_flag = NORMAL;

				//取走子进程的exit_status，并赋值给自己的child_exit_status
				p_proc_current->task.child_exit_status = proc_table[p_proc_current->task.info.child_process[i]].task.exit_status;

				//释放子进程的进程表项
				//proc_table[p_proc_current->task.info.child_process[i]].task.stat = IDLE; //deleted by mingxuan 2020-12-30
				free_PCB(&proc_table[p_proc_current->task.info.child_process[i]]);	//modified by mingxuan 2021-8-21

				//将自己的状态置为正常
				p_proc_current->task.we_flag = NORMAL;

				//父进程的孩子数减一
				p_proc_current->task.info.child_p_num--;

				//更新父进程的孩子数组
				p_proc_current->task.info.child_process[i] = 0;

				//只释放一个
				break;
			}
		}
		//如果没有子进程在ZOMBY
		if(exit_child_hanging == 0)
		{
			p_proc_current->task.we_flag = WAITING;
		}
	}

    return p_proc_current->task.child_exit_status;;

}