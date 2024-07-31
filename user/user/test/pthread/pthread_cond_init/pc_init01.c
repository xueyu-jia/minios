/*
 * 测试 pthread_cond_init 能否正确初始化
 *
 * 通过判断返回值是否为 0 判断是否初始化成功。
 *
 * 1. 测试带 condattr 参数的情况
 * 2. 测试 condattr 为 NULL 的情况
 *
 */
#include "usertest.h"

const char *test_name = "pthread_cond_init01";
const char *syscall_name = "pthread_cond_init";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int rval;
  pthread_cond_t cond1, cond2;

  // 带参数
  pthread_condattr_t condattr = {.name = "pthread_cond_init01 - condattr"};
  rval = pthread_cond_init(&cond1, &condattr);
  if (rval != 0) {
    info(&logger, "condattr return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  // 不带参数
  rval = pthread_cond_init(&cond2, NULL);
  if (rval != 0) {
    info(&logger, "NULL return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  rval = pthread_cond_destroy(&cond1);
  if (rval != 0) {
    warning(&logger, "failed to destroy cond1\n");
  }
  rval = pthread_cond_destroy(&cond2);
  if (rval != 0) {
    warning(&logger, "failed to destroy cond2\n");
  }

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
