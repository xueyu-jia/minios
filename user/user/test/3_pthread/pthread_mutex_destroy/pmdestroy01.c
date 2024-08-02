/*
 * 测试 pthread_mutex_destroy，应能将一个未被占有的 mutex 变为未初始化的状态，
 * 并返回 0.
 *
 */

#include "usertest.h"

const char *test_name = "pthread_mutex_destroy01";
const char *syscall_name = "pthread_mutex_destroy";

logging logger;

static pthread_mutex_t mutex1, mutex2;
// 宏初始化
static pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;

void setup() {
  int rval;

  logger_init(&logger, log_filename, test_name, LOG_INFO);
  info(&logger, "starting......\n");

  // 不带参数
  rval = pthread_mutex_init(&mutex1, NULL);
  if (rval != 0) {
    error(&logger, "pthread_mutex_init mutex1 failed, return %d\n", rval);
    cleanup();
    exit(TC_UNRESOLVED);
  }

  // 带参数
  pthread_mutexattr_t mutex_attr = {
      .name = "pthread_mutex_destroy01",
  };
  rval = pthread_mutex_init(&mutex2, &mutex_attr);
  if (rval != 0) {
    error(&logger, "pthread_mutex_init mutex2 failed, return %d\n", rval);
    cleanup();
    exit(TC_UNRESOLVED);
  }
}

void cleanup() { logger_close(&logger); }

void run() {
  int rval;

  // destroy mutex1
  rval = pthread_mutex_destroy(&mutex1);
  info(&logger, "pthread_mutex_destroy mutex1 return %d, expected 0\n", rval);
  if (rval != 0) {
    cleanup();
    exit(TC_FAIL);
  }

  // destroy mutex2
  rval = pthread_mutex_destroy(&mutex2);
  info(&logger, "%s: pthread_mutex_destroy mutex2 return %d, expected 0\n",
       test_name, rval);
  if (rval != 0) {
    cleanup();
    exit(TC_FAIL);
  }

  // destroy mutex3
  rval = pthread_mutex_destroy(&mutex3);
  info(&logger, "%s: pthread_mutex_destroy mutex3 return %d, expected 0\n",
       test_name, rval);
  if (rval != 0) {
    cleanup();
    exit(TC_FAIL);
  }

  info(&logger, "%s: passed\n", test_name);
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
