#include <pthread.h>
#include <stdio.h>
#include <stddef.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_t ntid;

int i = 1;
void thread1() {
    for (i = 1; i <= 9; i++) {
        pthread_mutex_lock(&mutex);
        if (i % 3 == 0)
            pthread_cond_signal(&cond); // 条件改变，发送信号通知t_b进程
        else
            printf("thread1:%d\n", i);
        pthread_mutex_unlock(&mutex);
        printf("up unlock mutex\n");
        sleep(1);
    }
}

void thread2() {
    while (i < 9) {
        pthread_mutex_lock(&mutex);
        if (i % 3 != 0) pthread_cond_wait(&cond, &mutex); // 等待
        printf("thread2:%d\n", i);
        pthread_mutex_unlock(&mutex);
        printf("down unlock mutex\n");
        sleep(1);
    }
}
int main(void) {
    pthread_create(&ntid, NULL, thread1, NULL);
    pthread_create(&ntid, NULL, thread2, NULL);
    sleep(3000);
    int sign = pthread_cond_destroy(&cond);
    if (sign == 0) {
        printf("cond is destroyed success!        \n");
    } else {
        printf("cond is destroyed fail!        \n");
    }
    sign = pthread_mutex_destroy(&mutex);
    if (sign == 0) {
        printf("mutex is destroyed success!        \n");
    } else {
        printf("mutex is destroyed fail!        \n");
    }
    printf("over!                                 \n");
    return 0;
}
