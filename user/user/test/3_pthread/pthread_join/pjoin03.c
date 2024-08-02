/*
 * 测试 pthread_join 调用成功的返回值是否为 0
 *
 * 1. 创建新线程
 * 2. 主线程调用 pthread_join 等待线程结束。pthread_join 应返回 0
 *
 */

#include "usertest.h"

const char *test_name = "pthread_join03";
const char *syscall_name = "pthead_join";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void *thread_func(void *arg) { pthread_exit(NULL); }

void run() {
  pthread_t thread_id;

  // 创建新线程
  int rval = pthread_create(&thread_id, NULL, thread_func, NULL);
  if (rval != 0) {
    info(&logger, "pthread_create return %d, thread_id: %d\n", rval, thread_id);
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

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}