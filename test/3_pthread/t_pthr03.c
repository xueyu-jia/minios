/*
    added by dzq 2023.6.15

    [Description]:主进程pthread_join先于子线程的pthread_exit发生

    主进程被挂起，因此最后打印main exit!



*/

#include <malloc.h>
#include <stdio.h>
#include <stddef.h>

int pid1, pid2, pid3;

void pthread_test3() {
    int k = 8;
    while (k--) {
        printf("pth3 ");
        sleep(150);
    }

    printf("thread 3 exit! ");
    pthread_exit(NULL);
}

void pthread_test2() {
    int k = 8;
    while (k--) {
        printf("pth2 ");
        sleep(120);
    }
    printf("thread 2 exit! ");
    pthread_exit(NULL);
}

void pthread_test1() {
    int k = 8;
    while (k--) {
        printf("pth1 ");
        sleep(100);
    }

    int *a = (int *)malloc(sizeof(int));
    *a = 5;

    printf("thread 1 exit! ");
    pthread_exit(NULL);

    // return;
}

int main(int arg, char *argv[]) {
    int k = 3;
    int pid1, pid2, pid3;
    // int *retval = 0;
    // char *retTEST2='0';
    pthread_create(&pid1, NULL, pthread_test1, NULL);
    pthread_create(&pid2, NULL, pthread_test2, NULL);
    pthread_create(&pid3, NULL, pthread_test3, NULL);

    while (k--) {
        printf("main is running! ");
        sleep(100);
    }

    pthread_join(pid1, NULL);
    pthread_join(pid2, NULL);
    pthread_join(pid3, NULL);

    printf("main exit!");
    exit(0);
}
