/*
    added by dzq 2023.6.15

    [Description]:测试pthread_create进程创建线程
    pthread_exit的正常退出、不传参

    [流程描述]：
    主进程创建线程，线程打印参数后调用pthread_exit退出，主进程调用pthread_join
    主进程再次创建线程，线程打印参数后退出，主进程调用pthread_join
    观察打印参数，两次创建的线程id相同，说明第一次创建的已退出，不再占用PCB



*/

#include <stdio.h>
#include <stddef.h>

void thread_fun(void *arg) {
    printf("pth1 print : I'm thread1, arg = %d. \n", *(int *)arg);
    int pid = get_pid();
    printf("pth1 print : thread1 pid = %d. \n", pid);
    pthread_exit(NULL);
    printf("pth1 print : exit error");
    // return (void *)111;
}

int main(int arg, char *argv[]) {
    pthread_t thread1;
    int ret1;
    int a = 10;
    int process = get_pid();
    printf("main print : process = %d \n", process);
    ret1 = pthread_create(&thread1, NULL, thread_fun, &a);
    printf("main print : thread1 = %d \n", thread1);

    sleep(100);
    pthread_join(thread1, NULL);
    printf("main print : pthread_create return = %d \n", ret1);

    printf("main print : ----do pthread_create again, after thread exit----\n");

    ret1 = pthread_create(&thread1, NULL, thread_fun, &a);
    printf("main print : thread1 = %d \n", thread1);
    sleep(100);
    pthread_join(thread1, NULL);
    printf("main print : pthread_create return = %d \n", ret1);

    exit(0);
}
