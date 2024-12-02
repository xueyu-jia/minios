/*
 * 测试 open 传入的文件名参数过长的情况
 *
 */
#include <usertest.h>

const char *test_name = "open05";
const char *syscall_name = "open";

logging logger;

const char *too_long_pathname =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijkl"
    "mnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwx"
    "yzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
int fd = -1;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  fd = open(too_long_pathname, O_RDWR | O_CREAT, I_RW);

  if (fd >= 0) {
    error(&logger, "open return %d, expected -1\n", fd);
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
