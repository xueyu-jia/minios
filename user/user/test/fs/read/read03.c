/*
 * 测试 read 能否正确返回读取的字节数
 *
 * 1. 新建一个文件，写入 "0123456789"，保存
 * 2. 重新打开文件，使用 read 读取 100 个字节的数据，应返回
 * 10，检查读取到的内容， 应为 "0123456789"
 *
 */

#include "usertest.h"

const char *test_name = "read03";
const char *syscall_name = "read";

logging logger;

const char *filename = "read03.txt";
char *write_str = "0123456789";
const int BUF_SIZE = 100;
int len;

int fd;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  len = strlen(write_str);
  fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
  SAFE_WRITE(fd, write_str, len);
  SAFE_CLOSE(fd);
  fd = SAFE_OPEN(filename, O_RDWR);
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
  info(&logger, "read() return %d, expected %d\n", n, len);
  if (n != len) {
    cleanup();
    exit(-1);
  }
  info(&logger, "read %s\n", (char *)read_buf);
  info(&logger, "expected %s\n", write_str);
  if (strncmp(write_str, read_buf, len) != 0) {
    cleanup();
    exit(-1);
  }

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
