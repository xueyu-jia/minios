/*********************************************************
*系统调用具体函数的实现
*
*
*
**********************************************************/

#include "const.h"
#include "string.h"
#include "time.h"
#include "buddy.h"
#include "kmalloc.h"
#include "ksignal.h"
#include "semaphore.h"
#include "syscall.h"

//modified by mingxuan 2021-8-14
//PUBLIC u32 do_malloc_4k()

/*======================================================================*
*                          sys_fork		add by visual 2016.4.8
*sys_fork()放在了fork.c文件中
*======================================================================*/

/*======================================================================*
                           sys_pthread		add by visual 2016.4.11
*sys_pthread()放在了pthread.c文件中
 *======================================================================*/

/*======================================================================*
                           sys_udisp_int		add by visual 2016.5.16
用户用的打印函数
 *======================================================================*/

//modified by mingxuan 2021-8-13
PUBLIC void sys_udisp_int()
{
	do_udisp_int(get_arg(1));
	return;
}

PUBLIC void do_udisp_int(int arg)
{
	disp_int(arg);
	return;
}

/*======================================================================*
                           sys_udisp_str		add by visual 2016.5.16
用户用的打印函数
 *======================================================================*/

//modified by mingxuan 2021-8-13
PUBLIC void sys_udisp_str()
{
	do_udisp_str(get_arg(1));
	return;
}

PUBLIC void do_udisp_str(char *arg)
{
	disp_str(arg);
	return;
}

/*======================================================================*
*                          sys_exec		被放在exec.c文件中
*======================================================================*/
/*======================================================================*
*                          sys_total_mem_size		added by wang 2021.8.21
*======================================================================*/
PUBLIC u32 kern_total_mem_size()
{
	u32 total_mem_size = 0;
	total_mem_size = kbud->current_mem_size + ubud->current_mem_size + kmem.current_mem_size;
	return total_mem_size;
}

PUBLIC u32 do_total_mem_size()
{
	return kern_total_mem_size();
}

PUBLIC u32 sys_total_mem_size()
{
	return do_total_mem_size();
}

PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {
	[_NR_get_ticks] =	sys_get_ticks,
	[_NR_get_pid]	= 	sys_get_pid,
	[_NR_malloc_4k] = 	sys_malloc_4k,		
	[_NR_free_4k] 	= 	sys_free_4k,
	[_NR_fork] 		=	sys_fork,
	[_NR_pthread_create] = sys_pthread_create,
	[_NR_udisp_int] =	sys_udisp_int,
	[_NR_udisp_str] = 	sys_udisp_str,
	[_NR_execve]	=	sys_execve,	
	[_NR_yield]		=	sys_yield,
	[_NR_sleep]		=	sys_sleep,
	[_NR_open]		=	sys_open,
	[_NR_close]		=	sys_close,
	[_NR_read]		= 	sys_read,
	[_NR_write]		=	sys_write,
	[_NR_lseek]		=	sys_lseek,
	[_NR_unlink]	=	sys_unlink,
	[_NR_creat]		=	sys_creat,
	[_NR_closedir]	=	sys_closedir,
	[_NR_opendir]	=	sys_opendir,
	[_NR_mkdir]		=	sys_mkdir,
	[_NR_rmdir]		=	sys_rmdir,
	[_NR_readdir]	=	sys_readdir,
	[_NR_chdir]		=	sys_chdir, 
	[_NR_getcwd]	=	sys_getcwd,
	[_NR_wait]		=	sys_wait,
	[_NR_exit]		=	sys_exit,
	[_NR_signal]	=	sys_signal,
	[_NR_sigsend]	=	sys_sigsend,
	[_NR_sigreturn]	=	sys_sigreturn,	
	[_NR_total_mem_size]	=	sys_total_mem_size,
	[_NR_shmget]	=	sys_shmget,
	[_NR_shmat]		=	sys_shmat,
	[_NR_shmdt]		=	sys_shmdt,
	[_NR_shmctl]	=	sys_shmctl,
	[_NR_shmmemcpy]	=	sys_shmmemcpy,
	[_NR_ftok]		=	sys_ftok,
	[_NR_msgget]	=	sys_msgget,	
	[_NR_msgsnd]	=	sys_msgsnd,	
	[_NR_msgrcv]	=	sys_msgrcv,
	[_NR_msgctl]	=	sys_msgctl,	
		// sys_test,
	[_NR_execvp]	=	sys_execvp,	
	[_NR_execv]		=	sys_execv,
	[_NR_pthread_self]	=	sys_pthread_self,
	[_NR_pthread_mutex_init]	=	sys_pthread_mutex_init,
	[_NR_pthread_mutex_destroy]	=	sys_pthread_mutex_destroy,
	[_NR_pthread_mutex_lock]	=	sys_pthread_mutex_lock,
	[_NR_pthread_mutex_unlock]	=	sys_pthread_mutex_unlock,
	[_NR_pthread_mutex_trylock]	=	sys_pthread_mutex_trylock,
	[_NR_pthread_cond_init]	=	sys_pthread_cond_init,
	[_NR_pthread_cond_wait]	=	sys_pthread_cond_wait,
	[_NR_pthread_cond_signal]	=	sys_pthread_cond_signal,
	[_NR_pthread_cond_timewait]	=	sys_pthread_cond_timewait,
	[_NR_pthread_cond_broadcast]=	sys_pthread_cond_broadcast,
	[_NR_pthread_cond_destroy]	=	sys_pthread_cond_destroy,
	[_NR_get_pid_byname]		=	sys_get_pid_byname,
	[_NR_mount]					=	sys_mount,
	[_NR_umount]				=	sys_umount,
	// [_NR_init_block_dev]		=	sys_init_block_dev,
	[_NR_pthread_exit]			=	sys_pthread_exit,
	[_NR_pthread_join]			=	sys_pthread_join,
	// [_NR_init_char_dev]			=	sys_init_char_dev,
	[_NR_get_time]				=	sys_get_time
	};

// #define TEST_FOR_SEMAPHORE
#ifdef TEST_FOR_SEMAPHORE
// #define TEST1
// #define TEST2
static int i = 0;
static int tmp = 0;
static int acc=0;
struct Semaphore print_sem,full,empty;
#define first_adder 1
#define second_adder 2
#define producer 3
#define consumer 4
#define MAXSIZE 5
#define EMPTY 0
int sum=0;
/*
*This function is used to ensure that 
*the first addition operation 
*can be completed completely
*without being interrupted by another
*added by cjj 2021-12-25
*/
int test1(void)
{
	int i=10;
	int tmp;
	while(i)
	{
		ksem_wait(&print_sem,1);//ensure add and print won't be intereupted
		tmp=sum;
		int j=1000000;
		while (j)
		{
			j--;
		}
		sum=tmp+1;
		i--;
	
	disp_str("sum1=");
	disp_int(sum);

	
		ksem_post(&print_sem,1);//Release lock and Exit conflict domain
	}

    return 0;
}
 /*
*This function is used to ensure that 
*the second addition operation 
*can be completed completely
*without being interrupted by another
*added by cjj 2021-12-25
*/
int test2(void)
{
	
    int i=10;
	int tmp;
    while(i)
    {
		ksem_wait(&print_sem,1);//ensure add and print won't be intereupted
		tmp=sum;
		int j=1000000;
		while (j)
		{
			j--;
		}
		sum=tmp+1;
		i--;
    // printf("sum2=%d\n",sum);
	disp_str("sum2=");
	disp_int(sum);

		ksem_post(&print_sem,1);//Release lock and Exit conflict domain
    }


    return 0;
}
/*
*This function is used to represent producers. 
*There are three producers in total
*/
int test_produce(int x)
{
	
    while(1)
    {
		int j =70000000;
		while (j)
		{
			j--;
		}
		
		ksem_wait(&full,1);
		ksem_wait(&print_sem,1);
		// printf("pth%d ",x);
		disp_str("pth");
		disp_int(x);
		disp_str("one product has been stored by");
		disp_int(x);
		disp_str("\n");
		// printf("one product has been stored by %d     \n",x);
		ksem_post(&print_sem,1);
		ksem_post(&empty,1);
    }
	while (1)
	{
		/* code */
	}
	
    return 0;
}
/*
*This function is used to represent consumer. 
*/
int test_consume(int x)
{

    while(1)
    {
		int j =10000000;
		while (j)
		{
			j--;
		}
		
#ifdef TEST1	
		int num=ksem_getvalue(&empty);

		if (ksem_trywait(&empty,num) !=-1)
		{
			ksem_wait(&print_sem,1);
			// printf("pth%d ",x);
			// printf("someone has bought %d product       \n",num);
			disp_str("pth");
			disp_int(x);
			disp_str("someone has bought ");
			disp_int(num);
			disp_str(" product\n");
			ksem_post(&print_sem,1);
			ksem_post(&full,num);
		}
#endif
#ifdef TEST2	
			ksem_wait(&empty,1);
			ksem_wait(&print_sem,1);
			disp_str("pth");
			disp_int(x);
			disp_str("someone has bought ");
			disp_int(1);
			disp_str(" product\n");
			ksem_post(&print_sem,1);
			ksem_post(&full,1);

#endif
    }

	
    return 0;
}
/*======================================================================*
*                          sys_test					added by cjj 2021.12.25	modified by Juan 2021.12.26
用于测试内核信号量的功能
*======================================================================*/

PUBLIC void sys_test()
{
	return do_test(get_arg(1));
}
PUBLIC void do_test(int function)
{
	return kern_test(function);
}

PUBLIC void kern_test(int function)
{
	
	if (!tmp)
	{
		tmp=1;
		ksem_init(&print_sem,1);					//Semaphore to ensure atomic operation
		ksem_init(&full,MAXSIZE);	//Semaphore used to judge whether the warehouse is full
		ksem_init(&empty,EMPTY);		//Semaphore used to judge whether the warehouse is empty
		// printf("1");
	}

	switch(function)
	{
		case	first_adder:    test1();
								break;

		case	second_adder:	test2();
								break;

		case 	producer:	  //	ksem_wait(print_sem,1);
								i++;
								test_produce(i);
								// ksem_post(print_sem,1);
								break;

		case 	consumer:	   //	ksem_wait(print_sem,1);
								i++; 
								test_consume(i);
								//ksem_post(print_sem,1);
								break;	

		default	: break;					
	}
	return ;

}
#endif /*TEST_FOR_SEMAPHORE*/