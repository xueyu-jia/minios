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
pthread_t ntid;

int global=0;
void test1()
{
	pthread_mutex_lock(&mutex);
	printf("test1: id=%d mutex.owner=%d   \n",pthread_self(),mutex.owner);//线程id,当前锁的持有线程id
	global++;
	printf("test1:global=%d   \n",global);
	sleep(1000);
	pthread_mutex_unlock(&mutex);
}
void test2()
{
	printf("test2: id=%d mutex.owner=%d   mutex.nusers=%d  \n",pthread_self(),mutex.owner,mutex.nusers);//线程id,当前锁的持有线程id,争用锁之前，存在多少线程需要争用该锁
	pthread_mutex_lock(&mutex);
	global++;
	printf("test2:global=%d   \n",global);
	pthread_mutex_unlock(&mutex);
}
void test3()
{
	printf("test3: id=%d mutex.owner=%d   mutex.nusers=%d  \n",pthread_self(),mutex.owner,mutex.nusers);//线程id,当前锁的持有线程id,争用锁之前，存在多少线程需要争用该锁
	pthread_mutex_lock(&mutex);
	global++;
	printf("test3:global=%d   \n",global);
	pthread_mutex_unlock(&mutex);
}
int main(int arg, char *argv[])
{
	attr.name = "test";
	pthread_mutex_init(&mutex, &attr);
	printf("pthread_mutex_init successful!\n");
	printf("mutex name is %s.\n", mutex.name);
	printf("Init over!\n");
	pthread_create(&ntid, NULL, test1, NULL);
	sleep(500);
	pthread_create(&ntid, NULL, test2, NULL);
	sleep(100);
	pthread_create(&ntid, NULL, test3, NULL);
	sleep(1000);
	pthread_mutex_destroy(&mutex);
	printf("mutex destroy success \n");
	printf("exe over!\n");
	return 0;
}