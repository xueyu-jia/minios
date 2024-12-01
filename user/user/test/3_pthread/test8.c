#include "../include/pthread.h"
#include "../include/stdio.h"
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
int count = 0, tid_creator = 0, tid_consumer = 0;
pthread_t ntid;

void consumer() {  //该函数减少count数值
                   // mutex.name="consumer";
  pthread_mutex_lock(&mutex);
  //当条件不满足时等待
  printf("in consumer count=%d  \n", count);
  while (count <= 0) {  //防止虚假唤醒
    // printf("consumer begin wait\n");
    pthread_cond_wait(&cond, &mutex);
    // printf("consumer end wait\n");
  }
  count--;
  printf("in consumer count=%d  \n", count);
  pthread_mutex_unlock(&mutex);
}
int main(int arg, char *argv[]) {
  int i = 0;
  printf("start\n");
  pthread_mutex_init(&mutex, NULL);
  printf("pthread_mutex_init successful!\n");
  pthread_cond_init(&cond, NULL);
  printf("pthread_cond_init successful!\n");
  printf("Init over!\n");
  tid_consumer = pthread_create(&ntid, NULL, consumer, NULL);
  sleep(2);
  pthread_mutex_lock(&mutex);

  count++;
  printf("in creator count=%d   \n", count);
  if (count > 0) {  //条件满足时发送信号
    pthread_cond_signal(&cond);
  }
  printf("in creator count=%d  \n", count);
  pthread_mutex_unlock(&mutex);
  sleep(1000);  //为了保证该子线程运行，需要让主线程sleep足够的时间
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
