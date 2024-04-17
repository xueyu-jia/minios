#include "../include/stdio.h"

int global = 1;
//char *str = "abcd";
char str[12];

void main(int arg,char *argv[])
{


	//printf("%s\n",str);		//deleted by mingxuan 2021-3-17
	printf("%s\n","abcd");
	printf("%d\n", global);	//deleted by mingxuan 2021-3-17
	
	//malloc(1024);

	if(0==mount("dev_sdb", "fat1", NULL, NULL, NULL))
	{
		printf("mount sdb to fat1\n");
	}

	printf(" %d ",get_pid());
	exit(get_pid());
}
// #include "stdio.h"
// // #define BUG1
// #define SIMPLE
// void pthread_test1(int *x)
// {
// 	test(1);
// 	while(1){}
// }

// void pthread_test2(int *x)
// {
// 	test(2);
// 	while(1){}
// }

// void pthread_test3(int *x)
// {
// 	test(3);
// }

// void pthread_test4(int *x)
// {
// 	test(4);
// }
// /*======================================================================*
//                           Syscall Pthread Test
// added by xw, 18/4/27
//  *======================================================================*/

// int main(int arg,char *argv[])
// {
// 	/* 
// 	* added by cjjx 2021-12-20
// 	* 测试get_pid,getppid,pthread_self的正确性
// 	*/
// 	// int i=0;
// 	// int *thread;
// 	// printf("now  ");
// 	// printf("%d\n",get_pid());
// 	// printf("parent  ");
// 	// printf("%d\n",getppid());
// 	// printf("thread  ");
// 	// printf("%d\n",pthread_self());
// 	// return 0;
// 	/* 
// 	* added by cjjx 2021-12-24
// 	* 自加操作问题测试
// 	*/
// 	// int i=1000;
// 	// int thread,thread2;
// 	// pthread_create((void*)thread,NULL,pthread_test1,NULL);
// 	// pthread_create((void*)thread2,NULL,pthread_test2,NULL);

// 	/* 
// 	* added by cjjx 2021-12-26
// 	* 生产者消费者问题测试
// 	*/
// 	int thread1,thread2,thread3,thread4,thread5;
// 	int temp = 1;

// #ifndef SIMPLE
// 	pthread_create((void*)thread1,NULL,pthread_test3,NULL);
// #ifndef BUG1
// 	for (int i = 0; i < 1000000; i++)
// 	{
// 		i++;
// 	}
// #endif
// 	pthread_create((void*)thread2,NULL,pthread_test3,NULL);
// #ifndef BUG1
// 	for (int i = 0; i < 1000000; i++)
// 	{
// 		i++;
// 	}
// #endif
// 	pthread_create((void*)thread3,NULL,pthread_test3,NULL);
// #ifndef BUG1
// 	for (int i = 0; i < 1000000; i++)
// 	{
// 		i++;
// 	}
// #endif
// 	pthread_create((void*)thread4,NULL,pthread_test4,NULL);
// #ifndef BUG1
// 	for (int i = 0; i < 1000000; i++)
// 	{
// 		i++;
// 	}
// #endif
// 	pthread_create((void*)thread5,NULL,pthread_test4,NULL);
// #endif

// #ifdef SIMPLE
// 	pthread_create(&thread1,NULL,pthread_test1,&temp);
// 	pthread_create(&thread2,NULL,pthread_test2,&temp);
// #endif

// while(1){}

// return 0;
// }