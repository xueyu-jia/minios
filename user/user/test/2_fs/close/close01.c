/*
 * 测试 close 能否正常关闭文件
 *
 *  打开一个文件，调用 close 关闭该文件描述符，close 应返回 0。
 *
 */

#include "usertest.h"

const char *test_name = "close01";
const char *syscall_name = "close";

logging logger;

char *filename = "close01.txt";
int fd;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  info(&logger, "starting......\n");

  fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
  info(&logger, "setup done\n");
}

void cleanup() {
  unlink(filename);
  logger_close(&logger);
}

void run() {
  int rval;
  rval = close(fd);
  info(&logger, "close(%d) return %d, expected: %d\n", fd, rval, 0);
  if (rval) {
    warning(&logger, "FAILED\n");
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
