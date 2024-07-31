/*
 * close 关闭一个非法的 fd 时，应返回 -1.
 */

#include "usertest.h"

const char *test_name = "close02";
const char *syscall_name = "close";

logging logger;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  info(&logger, "starting......\n");
}

void cleanup() { logger_close(&logger); }

void run() {
  int expected = -1;
  int bad_fd = -1;
  int ret = close(bad_fd);
  info(&logger, "close invalid fd return %d, expected: %d\n", ret, expected);
  if (ret != expected) {
    warning(&logger, "FAILED\n");
    cleanup();
    exit(-1);
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
