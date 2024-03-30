#include "const.h"
#include "proc.h"

/*======================================================================*
                      sys_wait            added by mingxuan 2021-1-6
 *======================================================================*/
//PUBLIC int sys_wait(void) 
PUBLIC int kern_wait(int *status) //wait返回的为子进程pid,子进程退出状态通过*status传递 modified by dongzhangqi 2023-4-20
{
	int child_pid = -1;
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
			if(p_proc_current->task.info.child_process[i] !=0 && proc_table[p_proc_current->task.info.child_process[i]].task.stat == KILLED)//统一PCB stat 20240314
			{
				exit_child_hanging = 1;

				//置子进程的状态为正常
				//proc_table[p_proc_current->task.info.child_process[i]].task.we_flag = NORMAL; 
				//proc_table[p_proc_current->task.info.child_process[i]].task.stat = KILLED; 

				//取走子进程的exit_status，并赋值给自己的child_exit_status
				p_proc_current->task.child_exit_status = proc_table[p_proc_current->task.info.child_process[i]].task.exit_status;

				/*added by dongzhangqi 2023.4.20
				修改wait的返回值，参考Linux的设计int wait(int *status)
				子进程的pid作为wait()的返回值，子进程的退出状态exit_status通过 *status来传递
				*/
				//取走子进程的pid，用作wait的返回值
				child_pid = proc_table[p_proc_current->task.info.child_process[i]].task.pid;

				//exit status通过*status传递
				if(status != NULL){
					*status = p_proc_current->task.child_exit_status;
				}


				//释放子进程的进程表项
				//proc_table[p_proc_current->task.info.child_process[i]].task.stat = IDLE; //deleted by mingxuan 2020-12-30
				free_PCB(&proc_table[p_proc_current->task.info.child_process[i]]);	//modified by mingxuan 2021-8-21

				//将自己的状态置为正常
				//p_proc_current->task.we_flag = NORMAL;

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
			//p_proc_current->task.we_flag = WAITING;

			wait_event(p_proc_current); //changed by dongzhangqi 2023.6.2
		}
	}

    //return p_proc_current->task.child_exit_status;;
	return child_pid;;
}

PUBLIC int do_wait(int *status) //wait返回的为子进程pid,子进程退出状态通过*status传递 modified by dongzhangqi 2023-4-20
{
	return kern_wait(status);
}

PUBLIC int sys_wait() //wait返回的为子进程pid modified by dongzhangqi 2023-4-20
{
	return do_wait((int*)get_arg(1));
}