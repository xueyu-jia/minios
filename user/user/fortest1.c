#include "../include/stdio.h"
#include "../include/pthread.h"
// #include "type.h"
// #include "const.h"
// #include "protect.h"
// #include "string.h"
// #include "proc.h"
// #include "global.h"
// #include "proto.h" 
// #include "spinlock.h"

pthread_mutex_t mutex;
pthread_mutexattr_t attr;
pthread_cond_t cond;
pthread_condattr_t cond_attr;
pthread_t ntid;

int count = 1, tid_creator = 0, tid_consumer = 0;
void test1(){
	int x=0;
	printf("test1 pid = %d    \n",get_pid());
	// printf("test1 ppid = %d    \n",get_ppid());
	while (1)
	{
		/* code */
	}
}
void test2(){
	int x=0;
	printf("test2 pid = %d    \n",get_pid());
	// printf("test2 ppid = %d    \n",get_ppid());
	while (1)
	{
		/* code */
	}
}
int main(int arg, char *argv[])
{
	printf("main pid = %d    \n",get_pid());
	sleep(20);
	// pthread_create(&ntid, NULL, test1, NULL);
	sleep(20);
	// pthread_create(&ntid, NULL, test2, NULL);
	sleep(20);

	// while (1)
	// {
	// 	/* code */
	// }
	exit(0);
	return 0;
}
