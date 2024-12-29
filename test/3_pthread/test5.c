#include <pthread.h>
#include <stdio.h>
#include <stddef.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutexattr_t attr;
pthread_t ntid;

void test1() {
    for (int i = 0; i < 4; i++) {
        // printf("%d\n",t);
        pthread_mutex_lock(&mutex);
        printf("mutex locked by main.The mutex owner= %d\n", mutex.owner);
        for (int j = 0; j < 3; j++) {
            printf("main sleep %d second.\n", j);
            sleep(1);
        }
        int sign = 0;
        if (i == 1) {
            sign = pthread_mutex_trylock(&mutex);
            if (sign == 0)
                printf("mutex trylock success\n");
            else
                printf("mutex trylock fail\n");
        } else if (i == 3) {
            sign = pthread_mutex_destroy(&mutex);
            if (sign == 0)
                printf("mutex destroyed success\n");
            else
                printf("mutex destroyed fail\n");
        }
        pthread_mutex_unlock(&mutex);
        printf("mutex unlocked by main.The mutex owner= %d\n", mutex.owner);
    }
    int sign = pthread_mutex_trylock(&mutex);
    if (sign == 0)
        printf("mutex trylock success\n");
    else
        printf("mutex trylock fail\n");
    pthread_mutex_unlock(&mutex);
    printf("mutex unlocked by main.The mutex owner= %d\n", mutex.owner);
    sign = pthread_mutex_destroy(&mutex);
    if (sign == 0)
        printf("mutex destroyed success\n");
    else
        printf("mutex destroyed fail\n");

    exit(0);
    while (1) { /* code */
    }
}
int main(int arg, char *argv[]) {
    printf("pthread_mutex_init successful!\n");
    printf("mutex name is %s.\n", mutex.name);
    printf("Init over!\n");
    pthread_create(&ntid, NULL, test1, NULL);
    sleep(100);
    exit(0);
    return 0;
}
