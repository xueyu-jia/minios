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
int count = 1, tid_creator = 0, tid_consumer = 0;
pthread_t ntid;

void consumer()
{ //该函数减少count数值
	printf("mutex is locked by consumer\n");
	pthread_mutex_lock(&mutex);
	//当条件不满足时等待
	printf("in consumer count=%d  \n", count);
	while (count <= 3)
	{ //防止虚假唤醒
		printf("consumer begin wait\n");
		int t=500;
		int sign_timeout=pthread_cond_timewait(&cond, &mutex,&t);//time 1000 ticks = 1s
		printf("in consumer count=%d  \n", count);
		printf("consumer end wait ");
		if(sign_timeout){
			printf("time out!\n");
			printf("in consumer count=%d  \n", count);
			pthread_mutex_unlock(&mutex);
			return ;
		}
		else {
			printf("in time!\n");
			break;
		}
	}
	count--;
	printf("in consumer count=%d  \n", count);
	printf("mutex is unlocked consumer\n");
	pthread_mutex_unlock(&mutex);
}
void consumer2()
{ //该函数减少count数值
	printf("mutex is locked by consumer2\n");
	pthread_mutex_lock(&mutex);
	//当条件不满足时等待
	printf("in consumer2 count=%d  \n", count);
	while (count <= 6)
	{ //防止虚假唤醒
		printf("consumer2 begin wait\n");
		int t=10;
		int sign_timeout=pthread_cond_timewait(&cond, &mutex,&t);//time 1000 ticks = 1s
		printf("in consumer2 count=%d  \n", count);
		printf("consumer2 end wait ");
		if(sign_timeout){
			printf("time out!\n");
			printf("in consumer2 count=%d  \n", count);
			pthread_mutex_unlock(&mutex);
			return ;
		}
		else {
			printf("in time!\n");
			break;
		}
	}
	count--;
	printf("in consumer2 count=%d  \n", count);
	printf("mutex is unlocked consumer2\n");
	pthread_mutex_unlock(&mutex);
}
int main(int arg, char *argv[])
{
	int i = 0;
	printf("start\n");
	attr.name = "mutex_test";
	cond_attr.name = "cond_test";
	pthread_mutex_init(&mutex, NULL);
	printf("pthread_mutex_init successful!\n");
	// printf("mutex name is %s\n.",mutex.name);
	pthread_cond_init(&cond, NULL);
	printf("pthread_cond_init successful!\n");
	// printf("cond name is %s\n.",cond.name);m
	printf("Init over!\n");
	pthread_create(&ntid, NULL, consumer, NULL);
	sleep(50);//由于线程内部的操作比较多，为了保证该线程先运行并且进入等待挂起，需要让主线程sleep足够的时间
	// tid_creator=pthread(creator);
	pthread_create(&ntid, NULL, consumer2, NULL);
	sleep(50);
	printf("mutex is locked by creator\n");
	pthread_mutex_lock(&mutex);
	int sign=0;
	while(1){
		count++;
		printf("in creator count=%d   \n", count);
		if (count > 6)
		{ 
			pthread_cond_broadcast(&cond);//条件满足时发送信号
			break;
		}
	}
	// pthread_cond_broadcast(&cond);//条件满足时发送信号
	printf("mutex is unlocked by creator\n");
	pthread_mutex_unlock(&mutex);

	sleep(2000);//为了保证该子线程运行，需要让主线程sleep足够的时间
	sign=pthread_cond_destroy(&cond);
	if(sign==0){
		printf("cond is destroyed success!        \n");
	}
	else{
		printf("cond is destroyed fail!        \n");
	}
	sign=pthread_mutex_destroy(&mutex);
	if(sign==0){
		printf("mutex is destroyed success!        \n");
	}
	else{
		printf("mutex is destroyed fail!        \n");
	}
	printf("over!                                 \n");
	return 0;
}

