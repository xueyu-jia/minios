#include <pthread.h>
#include <stdio.h>
#include <stddef.h>

pthread_mutex_t mutex;
pthread_mutexattr_t attr;
pthread_cond_t cond;
pthread_condattr_t cond_attr;
int count = 0, tid_creator = 0, tid_consumer = 0;
pthread_t ntid;

// 该函数增加count数值
//  void creator(){
//      printf("mutex is locked by creator\n");
//      pthread_mutex_lock(&mutex);
//      count++;
//      printf("in creator count=%d   \n",count);
//      if(count>0){//条件满足时发送信号
//          pthread_cond_signal(&cond);
//      }
//      printf("mutex is unlocked by creator\n");
//      pthread_mutex_unlock(&mutex);
//  }
void consumer() { // 该函数减少count数值
    printf("mutex is locked by consumer\n");
    pthread_mutex_lock(&mutex);
    // 当条件不满足时等待
    printf("in consumer count=%d  \n", count);
    while (count <= 0) { // 防止虚假唤醒
        printf("consumer begin wait\n");
        pthread_cond_wait(&cond, &mutex);
        printf("consumer end wait\n");
    }
    count--;
    printf("in consumer count=%d  \n", count);
    printf("mutex is unlocked consumer\n");
    pthread_mutex_unlock(&mutex);
    printf("mutex is destroyed in consumer\n");
    pthread_mutex_destroy(&mutex);
}
int main(int arg, char *argv[]) {
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
    sleep(
        5); // 由于线程内部的操作比较多，为了保证该线程先运行并且进入等待挂起，需要让主线程sleep足够的时间
    // tid_creator=pthread(creator);

    printf("mutex is locked by creator\n");
    pthread_mutex_lock(&mutex);
    count++;
    printf("in creator count=%d   \n", count);
    if (count > 0) { // 条件满足时发送信号
        pthread_cond_signal(&cond);
    }
    printf("mutex is unlocked by creator\n");
    pthread_mutex_unlock(&mutex);

    sleep(10); // 为了保证该子线程运行，需要让主线程sleep足够的时间
    printf("over!                       \n");
    printf("cond is destroyed success!  \n");
    pthread_cond_destroy(&cond);
    return 0;
}
