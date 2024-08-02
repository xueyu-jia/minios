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

pthread_t ntid;


void test2(){
	// printf ( "test2 pid is %d ppid=%d   \n " ,   get_pid (),get_ppid ());
    printf ( "test2 tid is %d \n " ,   pthread_self ());
}
void test1(){
	int x=0;
	// printf ( "test1 pid is %d ppid=%d   \n " ,   get_pid (),get_ppid ());
    printf ( "test1 tid is %d \n " ,   pthread_self ());

}
int main(int arg,char *argv[])
{
	int pid=fork();
	// printf("%d\n",pid);
	if(pid){
		sleep(50);
		pthread_create(&ntid, NULL, test1, NULL);
	}
	else{
		sleep(50);
		pthread_create(&ntid, NULL, test2, NULL);
	}
	// while(1);
	return 0;
}