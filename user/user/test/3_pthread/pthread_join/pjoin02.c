/*
 * 测试 pthread_join 一个已经结束的线程的情形
 *
 * 1. 创建新线程
 * 2. 主线程调用 pthread_join 等待线程结束
 * 3. 主线程再次调用 pthread_join。此时该线程已结束，pthread_join
 *      应返回 ESRCH i.e. 3
 * // 目前 MiniOS 的实现为返回 -1
 *
 */

#include "usertest.h"

const char *test_name = "pthread_join02";
const char *syscall_name = "pthead_join";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void *thread_func(void *arg) { pthread_exit(NULL); }

void run() {
  pthread_t thread_id;

  // 创建新线程
  int rval = pthread_create(&thread_id, NULL, thread_func, NULL);
  info(&logger, "pthread_create return %d, thread_id: %d\n", rval, thread_id);
  if (rval != 0) {
    cleanup();
    exit(TC_FAIL);
  }

  // 等待线程结束
  rval = pthread_join(thread_id, NULL);
  info(&logger, "pthread_join return %d, expected 0\n", rval);
  if (rval != 0) {
    cleanup();
    exit(TC_FAIL);
  }

  // 再次等待线程结束
  rval = pthread_join(thread_id, NULL);
  info(&logger, "pthread_join again return %d, expected 3\n", rval);
  // 目前 MiniOS 的实现为返回 -1
  if (rval != 3) {  // 3 or -1
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