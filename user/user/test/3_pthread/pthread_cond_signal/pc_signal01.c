/*
 * pthread_cond_signal 多个线程时，是否会出错。
 *
 *
 * failed: pthread_cond_wait 不会释放 mutex
 *
 * 1. 创建多个线程
 * 2. 主线程等待所有线程创建完毕
 * 3. 主线程发送信号
 *
 *
 *
 */
#include <usertest.h>

const char *test_name = "pthread_cond_signal01";
const char *syscall_name = "pthread_cond_signal";

logging logger;

#define THREAD_NUM 2

pthread_mutex_t mutex;
pthread_cond_t cond;

static volatile int start_count = 0;
static volatile int wakeup_count = 0;

void setup() {
  int rval;
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  info(&logger, "starting\n");

  rval = pthread_mutex_init(&mutex, NULL);
  printf("rval: %d\n", rval);
  rval = pthread_cond_init(&cond, NULL);
  printf("rval: %d\n", rval);
}

void cleanup() {
  int rval;

  rval = pthread_cond_destroy(&cond);
  if (rval != 0) {
    info(&logger, "pthread_cond_destroy cond return %d, expected 0\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  rval = pthread_mutex_destroy(&mutex);
  if (rval != 0) {
    info(&logger, "pthread_mutex_destroy mutex return %d, expected 0\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  logger_close(&logger);
}

void *thread_func(void *arg) {
  int rval;
  pthread_t tid = pthread_self();
  printf("thread: %d\n", tid);

  rval = pthread_mutex_lock(&mutex);
  if (rval != 0) {
    error(&logger, "thread[%d]: fail to acquire mutex, return %d\n", tid, rval);
    cleanup();
    exit(TC_UNRESOLVED);
  }

  info(&logger, "thread[%d]: starting...\n", tid);
  start_count++;

  // TODO 虚假唤醒
  // int origin_wakeup_count = wakeup_count;
  // printf("thread[%d]: wakeup_count: %d\n", tid, wakeup_count);
  // while (wakeup_count == 0) {
  printf("call pthread_cond_wait\n");
  rval = pthread_cond_wait(&cond, &mutex);
  if (rval != 0) {
    error(&logger, "thread[%d]: pthread_cond_wait return %d\n", tid, rval);
    cleanup();
    exit(TC_FAIL);
  }
  info(&logger, "thread[%d]: wakeup...\n", tid);
  // origin_wakeup_count = wakeup_count;
  wakeup_count++;

  rval = pthread_mutex_unlock(&mutex);
  if (rval != 0) {
    error(&logger, "thread[%d]: fail to release mutex, return %d\n", tid, rval);
    cleanup();
    exit(TC_FAIL);
  }

  pthread_exit(NULL);
}

void run() {
  int rval;
  pthread_t thids[THREAD_NUM];

  // 创建线程
  for (int i = 0; i < THREAD_NUM; i++) {
    rval = pthread_create(thids + i, NULL, thread_func, NULL);
    if (rval != 0) {
      error(&logger, "failed to create thread, i = %d, return %d\n", i, rval);
      cleanup();
      exit(TC_UNRESOLVED);
    }
  }

  // 等待所有线程开始
  while (start_count < THREAD_NUM) {
    printf("start_count: %d\n", start_count);
    sleep(100);
  }
  sleep(10);

  printf("start_count: %d\n", start_count);

  // 发信号前必须先获取 mutex
  rval = pthread_mutex_lock(&mutex);
  if (rval != 0) {
    error(&logger, "main thread: failed acquire mutex, return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }
  while (wakeup_count < THREAD_NUM) {
    info(&logger, "main thread: signal a condition\n");
    rval = pthread_cond_signal(&cond);
    if (rval != 0) {
      error(&logger, "main thread: failed to signal condition, return %d\n",
            rval);
      cleanup();
      exit(TC_FAIL);
    }
    sleep(10);
  }
  rval = pthread_mutex_unlock(&mutex);
  if (rval != 0) {
    error(&logger, "main thread: failed release mutex, return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  // 等待所有线程结束
  for (int i = 0; i < THREAD_NUM; i++) {
    rval = pthread_join(thids[i], NULL);
    if (rval != 0) {
      error(&logger, "failed to join thread[%d], return %d\n", i, rval);
      cleanup();
      exit(TC_FAIL);
    }
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
