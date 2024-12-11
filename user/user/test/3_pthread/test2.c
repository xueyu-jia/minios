#include <pthread.h>
#include <stdio.h>
// #include <type.h>
// #include <const.h>
// #include <protect.h>
// #include <string.h>
// #include <proc.h>
// #include <global.h>
// #include <proto.h>
// #include <spinlock.h>

pthread_t ntid;

void test4() {
    printf("test4 pid is %d \n ", get_pid());
    printf("test4 tid is %d \n ", pthread_self());
    sleep(10);
    exit(0);
}
void test3() {
    printf("test3 pid is %d \n ", get_pid());
    printf("test3 tid is %d \n ", pthread_self());
    sleep(10);
    exit(0);
}
void test1() {
    int x = 0;
    printf("test1 pid is %d \n ", get_pid());
    printf("test1 tid is %d \n ", pthread_self());
    printf("test1 :\n");
    pthread_create(&ntid, NULL, test3, NULL);
    pthread_create(&ntid, NULL, test4, NULL);
    sleep(10);
    exit(0);
}
int main(int arg, char *argv[]) {
    int i = 0;
    pthread_create(&ntid, NULL, test1, NULL);
    sleep(10);
    exit(0);
    return 0;
}
