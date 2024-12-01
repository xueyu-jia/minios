/*
 * 测试 pthread_cond_signal 能否唤醒正在等待中线程
 *
 * 1. 设置全局变量 flag 为 0
 * 2. 初始化 mutex 和 cond
 * 3. 主线程创建一个新线程，然后 sleep
 * 4. 新线程中使用 pthread_cond_wait 等待唤醒，直到 flag 为 1
 * 5. 主线程中设置 flag 为 1，使用 pthread_cond_signal 唤醒新线程
 * 6. 新线程被唤醒并结束线程
 * 7. 程序正常结束说明测试成功
 *
 */
#include "usertest.h"

const char *test_name = "pthread_cond_signal02";
const char *syscall_name = "pthread_cond_signal";

logging logger;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int flag = 0;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  info(&logger, "starting\n");
}

void cleanup() {
  int rval;

  rval = pthread_mutex_destroy(&mutex);
  if (rval != 0) {
    error(&logger, "pthread_mutex_destroy mutex return %d, expected 0\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  rval = pthread_cond_destroy(&cond);
  if (rval != 0) {
    error(&logger, "pthread_cond_destroy cond return %d, expected 0\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  logger_close(&logger);
}

// 线程函数
void *thread_func(void *arg) {
  int rval;
  pthread_t tid = pthread_self();

  // 必须先持有 mutex
  rval = pthread_mutex_lock(&mutex);
  if (rval != 0) {
    error(&logger, "thread[%d]: failed to acquire mutex, return %d\n", tid,
          rval);
    cleanup();
    exit(TC_FAIL);
  }

  while (!flag) {
    // 等待唤醒
    rval = pthread_cond_wait(&cond, &mutex);
    if (rval != 0) {
      error(&logger, "thread[%d]: pthread_cond_wait return %d\n", tid, rval);
      cleanup();
      exit(TC_FAIL);
    }
  }

  rval = pthread_mutex_unlock(&mutex);
  if (rval != 0) {
    error(&logger, "thread[%d]: failed to release mutex, return %d\n", tid,
          rval);
    cleanup();
    exit(TC_FAIL);
  }

  pthread_exit(NULL);
}

void run() {
  int rval;
  pthread_t thid;

  // 创建线程
  rval = pthread_create(&thid, NULL, thread_func, NULL);
  if (rval != 0) {
    error(&logger, "failed to create thread, return %d\n", rval);
    cleanup();
    exit(TC_UNRESOLVED);
  }

  // sleep 等待线程开始
  sleep(100);

  rval = pthread_mutex_lock(&mutex);
  if (rval != 0) {
    error(&logger, "main thread: pthread_mutex_lock failed, return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  flag = 1;
  info(&logger, "main thread: signal a condition\n");
  rval = pthread_cond_signal(&cond);
  if (rval != 0) {
    error(&logger, "main thread: failed to sinal condition, return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  rval = pthread_mutex_unlock(&mutex);
  if (rval != 0) {
    error(&logger, "main thread: pthread_mutex_unlock failed, return %d\n",
          rval);
    cleanup();
    exit(TC_FAIL);
  }

  // 等待所有线程结束
  rval = pthread_join(thid, NULL);
  if (rval != 0) {
    error(&logger, "failed to join thread, return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}