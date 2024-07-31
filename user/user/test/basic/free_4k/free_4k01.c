/*
 * 测试 free_4k 能回收一块内存
 *
 * 1. 使用 malloc_4k 申请一块内存
 * 2. 使用 free_4k 回收一块内存
 *
 */
#include "usertest.h"

const char *test_name = "malloc01";
const char *syscall_name = "malloc";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  char *buf = (char *)malloc_4k();

  int ret = free_4k(buf);
  info(&logger, "free_4k return %d\n", ret);

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}