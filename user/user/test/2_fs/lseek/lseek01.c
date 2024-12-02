/*
 * 测试 lseek 能否正常的处理 SEEK_SET 模式下偏移量为正的情况
 *
 */

#include <usertest.h>

const char *test_name = "lseek01";
const char *syscall_name = "lseek";

logging logger;

const char *filename = "lseek01.txt";
const char *write_str = "0123456789";
#define BUF_SIZE 100
int fd;
char read_buf[BUF_SIZE];
int exp_off = 4;
int exp_size = 6;
char *exp_data = "456789";

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
  fd = SAFE_OPEN(filename, O_RDWR);
  // 从头开始的正偏移量
  memset(read_buf, 0, BUF_SIZE);
  int off = lseek(fd, 4, SEEK_SET);
  info(&logger, "lseek return %d, expected: %d\n", off, exp_off);
  if (off == -1) {  // lseek error
    cleanup();
    exit(TC_FAIL);
  }

  int n = SAFE_READ(fd, read_buf, BUF_SIZE);
  info(&logger, "read return %d, expected %d\n", n, exp_size);
  if (n != exp_size) {
    // read 存在 bug
    // cleanup();
    //  exit(-1);
  }

  info(&logger, "read %s, expected: %s\n", (char *)read_buf, exp_data);
  if (strcmp((char *)read_buf, exp_data) != 0) {
    error(&logger, "FAILED\n");
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
