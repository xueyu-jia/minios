/*
 * execve 运行一个目录时应失败返回 -1
 *
 */

#include "usertest.h"

const char *test_name = "execve04";
const char *syscall_name = "execve";

logging logger;

const char *child_name = ".";

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int ret = execve(child_name, NULL, NULL);
  info(&logger, "execve return %d, exepcted %d\n", ret, -1);
  if (ret != -1) {
    info(&logger, "failed\n");
    exit(TC_FAIL);
  }
  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  run();
  exit(TC_PASS);
}
