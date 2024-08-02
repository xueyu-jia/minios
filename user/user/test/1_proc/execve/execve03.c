/*
 * execve 运行一个文件名长度大于 MAX_FILENAME_LEN 时应失败
 * execve 执行失败时应返回 -1
 *
 * execve 的返回值应该是一个 int 类型的值。然而 minios 的返回值是 u32 类型
 */

#include "usertest.h"

const char *test_name = "execve03";
const char *syscall_name = "execve";

logging logger;

const char *child_name = "a_name_longer_than_MAX_FILENAME_PATH";

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int ret = execve(child_name, NULL, NULL);
  if (ret != -1) {
    error(&logger, "execve return %d\n", ret);
    cleanup();
    exit(-1);
  }
  info(&logger, "execve return %d\n", ret);
  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(-1);
}
