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

//节点结构
struct node {
  int tot;
  int num[25];
  int head;
} Node;

//生产者
//线程同步--互斥锁
pthread_mutex_t mutex;
//阻塞线程--条件变量类型的变量
pthread_cond_t cond;
int producer_time = 1;
int customer_time = 2;
pthread_t ntid;

void producer() {
  int t = 100;
  for (int i = 1; i <= 10; i++) {
    //使用互斥锁保护共享数据
    pthread_mutex_lock(&mutex);
    Node.num[Node.tot++] = t++;
    printf("===== produce:%x,%d          \n", (unsigned int)pthread_self(),
           Node.num[Node.tot - 1]);
    //通知阻塞x的消费者线程，解除阻塞
    pthread_cond_signal(&cond);
    //解锁
    pthread_mutex_unlock(&mutex);

    sleep(2);  // sleep 1 s
  }
  while (1) {
    /* code */
  }
}
void customer() {
  for (int i = 1; i <= 10; i++) {
    //加锁
    pthread_mutex_lock(&mutex);
    if (Node.head == Node.tot) {
      //线程阻塞
      //该函数会对互斥锁进行解锁
      pthread_cond_wait(&cond, &mutex);
      //解除阻塞后，会对互斥锁做加锁操作
    }
    //当作消费者
    printf("-----customer %x ,%d          \n", (unsigned int)pthread_self(),
           Node.num[Node.head++]);
    //解锁
    pthread_mutex_unlock(&mutex);
    sleep(4);  // sleep 2 s
  }
  while (1) {
    /* code */
  }
}
int main() {
  printf("start        \n");
  //初始化
  pthread_mutex_init(&mutex, NULL);
  printf("mutex init success!        \n");
  pthread_cond_init(&cond, NULL);
  printf("cond init success!        \n");
  //创建生产者线程
  pthread_create(&ntid, NULL, customer, NULL);
  pthread_create(&ntid, NULL, producer, NULL);
  //创建消费者线程

  sleep(1000);
  //销毁
  int sign;
  sign = pthread_cond_destroy(&cond);
  printf("head=%d tail=%d           ", cond.head, cond.tail);
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
  exit(0);
  // while (1)
  // {
  // 	/* code */
  // }

  return 0;
}
