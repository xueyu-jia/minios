#include "../include/stdio.h"

// int global = 1;
// char str[12];

// void main(int arg,char *argv[])
// {


// 	//printf("%s\n",str);		//deleted by mingxuan 2021-3-17
// 	printf("%s\n","abcd");
// 	printf("%d\n", global);	//deleted by mingxuan 2021-3-17
	
// 	//malloc(1024);

// 	if(0==mount("dev_sdb", "fat1", NULL, NULL, NULL))
// 	{
// 		printf("mount sdb to fat1\n");
// 	}

// 	printf(" %d ",get_pid());
// 	exit(get_pid());
// }
// int main()
// {
// 	if(fork()!=0){
// 		for(int i = 0; i < 10; i++){
// 			printf("parent:%d\n", i);
// 		}
// 		int childStatues;
// 		wait(&childStatues);
// 		exit(0);
// 		return 0;
// 	}
// 	int ret;
// 	ret = execve("fstest.bin", NULL, NULL);
// 	if(ret != 0)	printf("execv error in userprograme\n");
// 	exit(0);
// 	return 0;
// }

void test_fucntion_1()
{
	if(fork()!=0){
		// parent
		for(unsigned int i = 0; i < 4294; i++){
			printf("a ");
		}
		exit(0);
	}
	//child
	for(unsigned int i = 0; i < 4294; i++){
		printf("b ");
	}
	exit(0);
}

void test_fucntion_2()
{
	if(fork()!=0){
		// parent
		
		for(unsigned int i = 0; i < 4294; i++){
			printf("a ");
		}
		exit(0);
	}
	//child
	nice(-6);
	for(unsigned int i = 0; i < 4294; i++){
		printf("b ");
	}
	exit(0);
}

void test_fucntion_3()
{
	if(fork()!=0){
		// parent
		nice(-6);
		int a = 0;
		for(int j = 0; j < 10; j++)
		for(int x = 0; x < 10; x++){
			int a = 0;
			for(unsigned int i = 0; i < 429400; i++){
			// printf("a ");
			a++;
			}
		}
		
		proc_msg msg;
		get_proc_msg(&msg);
		print_PCB(&msg);
		exit(0);
	}
	//child
	int a = 0;
	for(unsigned int i = 0; i < 429400; i++){
		// printf("b ");
		a++;
	}
	exit(0);
}

void test_fucntion_4()
{
	if(fork()==0){
		// chile
		set_rt(TRUE);
		for(unsigned int i = 0; i < 4294; i++){
			printf("a ");
		}
		exit(0);
	}
	//parent
	for(unsigned int i = 0; i < 4294; i++){
		printf("b ");
	}
	exit(0);
}

void test_fucntion_5()
{
	if(fork()!=0){
		// parent
		nice(-6);
		set_rt(TRUE);
		int a = 0;
		for(int j = 0; j < 10; j++)
		for(int x = 0; x < 10; x++){
			int a = 0;
			for(unsigned int i = 0; i < 429400; i++){
			// printf("a ");
			a++;
			}
		}
		
		proc_msg msg;
		get_proc_msg(&msg);
		print_PCB(&msg);
		exit(0);
	}
	//child
	int a = 0;
	for(unsigned int i = 0; i < 429400; i++){
		// printf("b ");
		a++;
	}
	exit(0);
}

void print_PCB(proc_msg *msg)
{
	printf("\npid = %d; nice = %d; vruntime = %d; cpu_time = %d\n",	\
			msg->pid, msg->nice, msg->vruntime, msg->sum_cpu_use);
}
int main()
{
	test_fucntion_5();
}