/*
 * 测试 pthread_broad_cast 能否唤醒所有使用 pthread_cond_wait 等待的线程
 *
 * failed: pthread_cond_wait 不会释放 mutex
 *
 * 1. 创建多个线程。每个线程都调用 pthread_cond_wait 等待唤醒。
 * 2. 主线程等待所有线程创建完毕
 * 3. 主线程调用 pthread_cond_broadcast 唤醒线程
 * 4. 所有线程都应被唤醒。每个线程检查是否获取到了 mutex
 *
 */
#include <usertest.h>

const char *test_name = "pthread_cond_broadcast01";
const char *syscall_name = "pthread_cond_broadcast";

logging logger;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

#define THREAD_NUM 10
static int start_count = 0;
static int wakeup_count = 0;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  info(&logger, "starting\n");
}

void cleanup() {
  int rval;

  rval = pthread_mutex_destroy(&mutex);
  if (rval != 0) {
    error(&logger, "failed to destroy mutex! return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  rval = pthread_cond_destroy(&cond);
  if (rval != 0) {
    error(&logger, "failed to destroy cond! return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  logger_close(&logger);
}

void *thread_func(void *arg) {
  int rval;
  pthread_t tid = pthread_self();

  rval = pthread_mutex_lock(&mutex);
  if (rval != 0) {
    error(&logger, "thread[%d]: fail to acquire mutex, return %d\n", tid, rval);
    cleanup();
    exit(TC_UNRESOLVED);
  }
  info(&logger, "thread[%d]: starting...\n", tid);
  start_count++;

  info(&logger, "thread[%d]: waiting for condition\n", tid);
  rval = pthread_cond_wait(&cond, &mutex);
  if (rval != 0) {
    error(&logger, "thread[%d]: pthread_cond_wait return %d\n", tid, rval);
    cleanup();
    exit(TC_UNRESOLVED);
  }

  info(&logger, "thread[%d]: wakeup...\n", tid);
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
  info(&logger, "main thread: creating threads\n");
  for (int i = 0; i < THREAD_NUM; i++) {
    rval = pthread_create(thids + i, NULL, thread_func, NULL);
    if (rval != 0) {
      error(&logger, "failed to create thread, i = %d, return %d\n", i, rval);
      cleanup();
      exit(TC_UNRESOLVED);
    }
  }

  // 等待所有线程启动
  info(&logger, "main thread: waiting for threads start...\n");
  while (start_count < THREAD_NUM) {
    sleep(10);
  }
  sleep(10);

  info(&logger, "main thread: all threads started\n");

  // 获取 mutex 以检查所有线程是否都阻塞在 pthread_cond_wait
  // 如果获取不到 mutex，则说明某个线程中的 pthread_cond_wait 没有释放 mutex
  rval = pthread_mutex_lock(&mutex);
  if (rval != 0) {
    error(&logger, "main thread: failed to acquire mutex, return %d\n", rval);
    cleanup();
    exit(TC_UNRESOLVED);
  }
  info(&logger, "main thread: unlock mutex\n");
  rval = pthread_mutex_unlock(&mutex);
  if (rval != 0) {
    warning(&logger, "main thread: pthread_mutex_unlock failed, return %d\n",
            rval);
    cleanup();
    exit(TC_UNRESOLVED);
  }

  // broadcast
  info(&logger, "main thread: broadcast the condition\n");
  rval = pthread_cond_broadcast(&cond);
  if (rval != 0) {
    error(&logger,
          "main thread: pthread_cond_broadcast return %d, expected %d\n", rval,
          0);
    cleanup();
    exit(TC_FAIL);
  }
  sleep(1000);

  // 检查所有线程是否都被唤醒
  if (wakeup_count < THREAD_NUM) {
    error(&logger,
          "main thread: not all threads were wakened. wakeup_count: %d, "
          "expected: %d\n",
          wakeup_count, THREAD_NUM);
    cleanup();
    exit(TC_FAIL);
  }

  for (int i = 0; i < THREAD_NUM; i++) {
    rval = pthread_join(thids[i], NULL);
    if (rval != 0) {
      warning(&logger, "main thread: pthread_join return %d\n", rval);
    }
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
