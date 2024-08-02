/*
 * lseek 到一个指定位置，并从该位置写入数据，看是否能覆盖正确位置的原始数据
 *
 * 1. 打开文件，写入 "0123456789"，关闭文件
 * 2. 再次打开文件，lseek 到文件开关偏移量为 5 的位置
 * 3. 使用 write 写入 "abcd"，关闭文件
 * 4. 再次打开文件，读取文件的内容，看是否为 "01234abcd9"
 *
 */

#include "usertest.h"

const char *test_name = "lseek08";
const char *syscall_name = "lseek";

logging logger;

#define BUF_SIZE 100

const char *orign_str = "0123456789";
const char *write_str = "abcd";
const char *exp_str = "01234abcd9";
const int exp_size = 10;
const char *filename = "lseek08.txt";
char read_buf[BUF_SIZE];
int fd;


void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  fd = SAFE_OPEN(filename, O_RDWR | O_CREAT);
  SAFE_WRITE(fd, orign_str, strlen(orign_str));
  SAFE_CLOSE(fd);
  memset(read_buf, 0, BUF_SIZE);
}

void cleanup() {
  SAFE_CLOSE(fd);
  unlink(filename);

  logger_close(&logger);
}

void run() {
  fd = SAFE_OPEN(filename, O_RDWR);
  int off = lseek(fd, 5, SEEK_SET);
  info(&logger, "lseek return %d, expected %d\n", off, 5);
  if (off != 5) {
    cleanup();
    exit(-1);
  }
  SAFE_WRITE(fd, write_str, strlen(write_str));
  SAFE_CLOSE(fd);

  fd = SAFE_OPEN(filename, O_RDWR);
  int n = SAFE_READ(fd, read_buf, BUF_SIZE);
  info(&logger, "read return %d, expected %d\n", n, exp_size);
  if (n != strlen(exp_str)) {
    // cleanup();
    // exit(-1);
  }

  info(&logger, "read %s, expected: %s\n", (char *)read_buf, exp_str);
  if (strcmp((char *)read_buf, exp_str) != 0) {
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
