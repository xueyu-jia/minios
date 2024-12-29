/*
    added by dzq 2023.6.15

    [Description]:测试pthread_create子线程创建线程
    pthread_exit正常退出、不传参

    观察打印参数是否正常




*/

#include <stdio.h>
#include <stddef.h>

void thread_fun2(void *arg) {
    printf("pth2 print : I'm thread2, arg = %d. \n", *(int *)arg);

    pthread_exit(NULL);
    printf("pth2 print : exit error");
}

void thread_fun1(void *arg) {
    printf("pth1 print : I'm thread1, arg = %d. \n", *(int *)arg);
    pthread_t thread2;
    int ret1;
    int a = 9;
    ret1 = pthread_create(&thread2, NULL, thread_fun2, &a);
    printf(" pth1 print : thread2 = %d \n", thread2);

    sleep(100);
    pthread_join(thread2, NULL);

    printf("pth1 print : pthread_create return = %d \n", ret1);

    pthread_exit(NULL);
    printf("pth1 print : exit error");
}

int main(int arg, char *argv[]) {
    pthread_t thread1;
    int ret1;
    int a = 10;
    int process = get_pid();
    printf("main print : process = %d \n", process);
    ret1 = pthread_create(&thread1, NULL, thread_fun1, &a);
    printf("main print : thread1 = %d \n", thread1);

    sleep(100);
    pthread_join(thread1, NULL);

    // sleep(20);
    printf("main print : pthread_create return = %d \n", ret1);
    exit(0);
}
