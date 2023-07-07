/*
    added by dzq 2023.5.13
    
    [Description]:

    

*/


#include "../include/stdio.h"


/*
typedef struct two_int
{
	int int1;
	int int2;
} *p_two_int;

// 普通函数，有参数和返回值
void *thread_fun(void *arg)
{
	printf("I'm thread1, arg = %d. \n", *(int *)arg);
	return (void *)111;
}

// 用pthread_exit退出
void *thread_fun2(void *arg)
{
	int a = 333;
	printf("I'm thread2. \n");
	pthread_exit((void *)"THREAD 2 pthread_exit");
	return (void *)a;
}

// 自定义参数
void *thread_fun3(void *arg)
{
	int ans = ((p_two_int)arg)->int1 + ((p_two_int)arg)->int2;
	printf("I'm thread3, ans = %d. \n", ans);
	return (void*)ans;
}

void main(int arg, char *argv[]){
    pthread_t thread1, thread2, thread3;
    int ret1, ret2, ret3;
    void *retval1, *retval2, *retval3;
    int a = 10;

    ret1 = pthread_create(&thread1, NULL, thread_fun, &a);
	ret2 = pthread_create(&thread2, NULL, thread_fun2, NULL);
    
    pthread_join(thread1, &retval1);
	pthread_join(thread2, &retval2);

	struct two_int arg3;
	arg3.int1 = 5;
	arg3.int2 = 6;
	ret3 = pthread_create(&thread3, NULL, thread_fun3, &arg3);
	pthread_join(thread3, &retval3);

	printf("I'm main, ret1 = %d, retval1 = %d \n", ret1, (int)retval1);
	printf("\tthread2 = %d, retval2 = %s \n", thread2, (char *)retval2);
	printf("\tthread3 = %d, retval3 = %d \n", thread3, (int)retval3);

}
*/

/*

void thread_fun(void *arg)
{
	printf("I'm thread1, arg = %d. \n", *(int *)arg);
	int pid = get_pid();
	printf("thread1 pid = %d. \n", pid);
	int *a = 5;
    pthread_exit((void*)a);
	printf("exit error");
	//return (void *)111;
}

void thread_fun2()
{
	int x=0;
	printf("test1 pid = %d    \n",get_pid());
    while(1){

    }

}

int main(int arg, char *argv[]){
    pthread_t thread1;
    //int *retval1 = 4;
	int *retval1 = 4;
    int ret1;
    int a = 10;
	int process = get_pid();
	printf("process = %d \n", process);
    ret1 = pthread_create(&thread1, NULL, thread_fun, &a);
	printf("thread1 = %d \n", thread1);

	sleep(100);
	pthread_join(thread1, (void*)&retval1);

    //sleep(20);
    printf("I'm main, ret1 = %d \n", ret1);
	printf("retval1 = %d \n", retval1);
    exit(0);
}
*/


int pid1,pid2,pid3;

void pthread_test3()
{
	int k = 3;
	while(k--)
	{
		udisp_str("pth3 ");	
		sleep(150);
		
	}

	udisp_str("thread 3 exit! ");
	pthread_exit(NULL);
}


void pthread_test2()
{
	int k = 3;
	while(k--)
	{
		udisp_str("pth2 ");
		sleep(120);
	}
	udisp_str("thread 2 exit! ");
	pthread_exit(NULL);
}

void pthread_test1()
{
	int i;
	int k = 3;
	while(k--)
	{
		udisp_str("pth1 ");
		sleep(100);
		
	}
	
	int *a= (int *)malloc(sizeof(int));
    *a=5;

	udisp_str("thread 1 exit! ");	
	pthread_exit(NULL);
	
	//return;
}

int main(int arg,char *argv[])
{
	int k = 8;
	int pid1,pid2,pid3;
	//int *retval = 0;
	//char *retTEST2='0';
	pthread_create(&pid1, NULL, pthread_test1, NULL);
	pthread_create(&pid2, NULL, pthread_test2, NULL);
	pthread_create(&pid3, NULL, pthread_test3, NULL);	

	while(k--)
	{	
		udisp_str("main is running! ");
		sleep(100);
	}
	
	pthread_join(pid1,NULL);
	//udisp_str("the retval of pth1 is:");
	//udisp_int(retval);
	pthread_join(pid2,NULL);
	pthread_join(pid3,NULL);
	

	
	udisp_str("main exit!");
	exit(0);
}
