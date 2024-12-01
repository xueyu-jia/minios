/*
 * 测试 write 出错时，是否会破坏已写入数据
 * 1. 创建一个新文件，先写入 100 字节数据。
 * 2. 调用 write 并给错误的参数，使 write 出错
 * 3. 调用 close 关闭文件
 * 4. 重新打开文件并读取文件内容，检查文件内容和已写入的数据是否一样
 *  // how to build a bad adress to make write corrupt?
 *  // use `(void *) -1` will make minios restart
 */

#include "usertest.h"

const char *test_name = "write03";
const char *syscall_name = "write";

logging logger;

const char *filename = "write03.txt";
#define BUF_SIZE 100
char write_buf[BUF_SIZE];
int fd;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  memset(write_buf, '1', BUF_SIZE);
  fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
  SAFE_WRITE(fd, write_buf, BUF_SIZE);
}

void run() {
  char read_buf[BUF_SIZE];
  memset(read_buf, 0, BUF_SIZE);
  // TODO make write corrupt
  // int n = write(fd, (void *)-1, -1); // bad write
  int n = write(fd, write_buf, -1);  // bad write
  if (n != -1) {
    info(&logger, "failed to fail\n");
    cleanup();
    exit(TC_FAIL);
  }
  SAFE_CLOSE(fd);
  fd = SAFE_OPEN(filename, O_RDWR);
  n = read(fd, read_buf, BUF_SIZE);
  if (n != BUF_SIZE) {
    info(&logger, "read return %d, expected %d\n", n, BUF_SIZE);
    cleanup();
    exit(TC_FAIL);
  }
  if (memcmp((void *)read_buf, (void *)write_buf, BUF_SIZE) != 0) {
    info(&logger, "write corrupted file!\n");
    info(&logger, "test failed\n");
    cleanup();
    exit(TC_FAIL);
  }
  info(&logger, "passed\n");
}

void cleanup() {
  if (fd > 0) {
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
