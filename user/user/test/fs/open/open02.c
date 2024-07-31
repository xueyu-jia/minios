/*
 * 测试 open 能否正常创建文件
 *
 */
#include "usertest.h"

const char *test_name = "open02";
const char *syscall_name = "open";

logging logger;

const char *filename = "open02.txt";
int fd = -1;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  info(&logger, "starting......\n");
}

void run() {
  info(&logger, "create file %s\n", filename);
  fd = open(filename, O_RDWR | O_CREAT, I_RW);
  if (fd < 0) {
    error(&logger, "fd return %d, expected an valid fd\n", fd);
    cleanup();
    exit(TC_FAIL);
  }
  info(&logger, "PASSED\n");
}

void cleanup() {
  if (fd >= 0) {
    SAFE_CLOSE(fd);
    unlink(filename);
  }
  logger_close(&logger);
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
