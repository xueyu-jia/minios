/*
 * 测试 lseek 能否正常的处理 SEEK_END 模式下偏移量为负的情况
 */

#include "usertest.h"

const char *test_name = "lseek02";
const char *syscall_name = "lseek";

logging logger;

const char *filename = "lseek02.txt";
const char *write_str = "0123456789";
#define BUF_SIZE 100
int fd;
char read_buf[BUF_SIZE];

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  // 创建测试文件
  fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
  SAFE_WRITE(fd, write_str, strlen(write_str));
  SAFE_CLOSE(fd);
}

void cleanup() {
  SAFE_CLOSE(fd);
  unlink(filename);

  logger_close(&logger);
}

void run() {
  int exp_off = 7;
  int exp_size = 3;
  char *exp_data = "789";

  fd = SAFE_OPEN(filename, O_RDWR);
  memset(read_buf, 0, BUF_SIZE);
  int off = lseek(fd, -3, SEEK_END);
  info(&logger, "lseek return %d, expected: %d\n", off, exp_off);
  if (off == -1) {  // lseek error
    cleanup();
    exit(TC_FAIL);
  }
  int n = SAFE_READ(fd, read_buf, BUF_SIZE);
  info(&logger, "read return %d, expected %d\n", n, exp_size);
  if (n != exp_size) {
    // cleanup();
    //  exit(-1);
  }
  info(&logger, "read %s, expected: %s\n", (char *)read_buf, exp_data);
  if (strcmp((char *)read_buf, exp_data) != 0) {
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
