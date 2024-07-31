/*
 * 测试 read 能否从文件中读取数据。
 *
 * 1. 创建一个文件，写入 100 个 'a'
 * 2. 使用 read 读取该文件，检查是否正确读取到 100 个 'a'
 *
 */

#include "usertest.h"

const char *test_name = "read01";
const char *syscall_name = "read";

logging logger;

const char *filename = "read01.txt";
const int BUF_SIZE = 100;

int fd;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  info(&logger, "setup...\n");
  char write_buf[BUF_SIZE];
  memset(write_buf, 'a', BUF_SIZE);
  fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
  SAFE_WRITE(fd, write_buf, BUF_SIZE);
  lseek(fd, 0, SEEK_SET);
  info(&logger, "setup done\n");
}

void cleanup() {
  SAFE_CLOSE(fd);
  unlink(filename);
  logger_close(&logger);
}

void run() {
  char read_buf[BUF_SIZE];
  memset(read_buf, 0, BUF_SIZE);
  int n = read(fd, read_buf, BUF_SIZE);
  info(&logger, "read() return %d, expected %d\n", n, BUF_SIZE);
  if (n != BUF_SIZE) {
    cleanup();
exit(TC_FAIL);
  }
  for (int i = 0; i < BUF_SIZE; i++) {
    if (read_buf[i] != 'a') {
      error(&logger, "read %c, expected %c\n", read_buf[i], 'a');
      cleanup();
exit(TC_FAIL);
    }
  }
  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}