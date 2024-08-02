/*
 * 测试 read 读取到 eof 时，是否会返回 0
 *
 * 1. 新建一个文件，写入一个字符 'a'，保存
 * 2. 重新打开该文件，读取一个字节，应读取到 'a'
 * 3. 继续读取一个字节，此时读到 EOF，read 应返回
 * 0，且不会修改传入的地址中的数据
 *
 */

#include "usertest.h"

const char *test_name = "read04";
const char *syscall_name = "read";

logging logger;

const char *filename = "read04.txt";
char *write_str = "a";
const int BUF_SIZE = 100;

int fd;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  info(&logger, "prepare for read04.txt\n");
  fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
  SAFE_WRITE(fd, write_str, strlen(write_str));
  SAFE_CLOSE(fd);
  fd = SAFE_OPEN(filename, O_RDWR);
  info(&logger, "setup done\n");
}

void cleanup() {
  SAFE_CLOSE(fd);
  unlink(filename);
  logger_close(&logger);
}

void run() {
  // 读取一个字符
  char c = 'A';
  int n = read(fd, &c, sizeof(c));
  info(&logger, "read return %d, expected %d\n", n, 1);
  info(&logger, "read character %c, expected %c\n", c, 'a');
  if (n != sizeof(c) || c != 'a') {
    cleanup();
    exit(TC_FAIL);
  }

  // 继续读取一个字符
  c = 'A';
  n = read(fd, &c, sizeof(c));
  info(&logger, "read return %d, expected %d\n", n, 0);
  info(&logger, "character c: %c, expected %c\n", c, 'A');
  if (n != 0 || c != 'A') {
    cleanup();
    exit(TC_FAIL);
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
