/*
 * pthread_mutex_trylock 一个已被占有的 mutex 应返回 -1
 *
 * 1. 初始化一个 mutex
 * 2. 创建一个线程并使用 pthread_mutex_lock 获取该 mutex
 * 3. 主线程中使用 pthread_mutex_trylock 获取该 mutex，应返回 -1.
 *    （Linux 为返回 EBUSY i.e. 16）
 * 4. 创建另外一个线程使用 pthread_mutex_unlock 释放该 mutex
 * 5. 主线程中使用 pthread_mutex_trylock 获取该 mutex，应能正确获取该 mutex
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_trylock01";
const char *syscall_name = "pthread_mutex_trylock";

logging logger;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void *thread_lock_func() {
  int rval = pthread_mutex_lock(&mutex);
  if (rval != 0) {
    info(&logger, "thread: pthread_mutex_lock: %d\n", rval);
  }
  pthread_exit(NULL);
}

void *thread_unlock_func() {
  int rval = pthread_mutex_unlock(&mutex);
  if (rval != 0) {
    info(&logger, "thread: pthread_mutex_unlock: %d\n", rval);
  }
  pthread_exit(NULL);
}

void run() {
  int rval;
  pthread_t tid1, tid2;

  rval = pthread_create(&tid1, NULL, thread_lock_func, NULL);
  if (rval != 0) {
    info(&logger, "error: pthread_create return %d\n");
    cleanup();
    exit(TC_FAIL);
  }

  pthread_join(tid1, NULL);

  rval = pthread_mutex_trylock(&mutex);
  info(&logger, "pthread_mutex_trylock return %d, expected -1\n", rval);
  if (rval != -1) {
    cleanup();
    exit(TC_FAIL);
  }

  rval = pthread_create(&tid2, NULL, thread_unlock_func, NULL);
  if (rval != 0) {
    info(&logger, "error: pthread_create return %d\n");
    cleanup();
    exit(TC_FAIL);
  }

  pthread_join(tid2, NULL);

  rval = pthread_mutex_trylock(&mutex);
  info(&logger, "pthread_mutex_trylock return %d, expected 0\n", rval);
  if (rval != 0) {
    cleanup();
    exit(TC_FAIL);
  }

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
