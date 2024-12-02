/*
 * 测试 write 能否写入数据到文件中。
 *
 * 1. 创建一个文件，写入内容
 * 2. 检查 write 的返回值是否和写入的字节数相等
 * 3. 用 read 读取数据，检查读出和写入的数据是否相同
 */

#include <usertest.h>

const char *test_name = "write01";
const char *syscall_name = "write";

logging logger;

const char *filename = "write01.txt";
const int BUF_SIZE = 100;
int fd;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
}

void cleanup() {
  SAFE_CLOSE(fd);
  unlink(filename);
  logger_close(&logger);
}

void run() {
  char buf[BUF_SIZE];
  memset(buf, 'w', BUF_SIZE);
  int n = write(fd, buf, BUF_SIZE);
  if (n == -1) {
    error(&logger, "failed to write, return %d\n", n);
    cleanup();
    exit(TC_FAIL);
  } else if (n != BUF_SIZE) {
    error(&logger, "partial write, return %d, expected %d\n", n, BUF_SIZE);
    cleanup();
    exit(TC_FAIL);
  }

  if (lseek(fd, 0, SEEK_SET) != 0) {
    error(&logger, "seek error\n");
    cleanup();
    exit(TC_UNRESOLVED);
  }
  char read_buf[BUF_SIZE];
  memset(read_buf, 0, BUF_SIZE);
  read(fd, read_buf, BUF_SIZE);
  for (int i = 0; i < BUF_SIZE; i++) {
    if (read_buf[i] != 'w') {
      error(&logger, "read %c, expected: %c\n", read_buf[i], 'w');
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
  exit(0);
}
