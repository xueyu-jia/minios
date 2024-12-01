/*
 * 测试 getcwd 功能是否正常。getcwd 会返回一个包含当前工作目录绝
 * 对路径的字符串，并写入到由参数传入的 buf 中。
 *
 * getcwd 返回的指针值应与传入的 buf 地址值相同。
 *
 */

#include "usertest.h"

const char *test_name = "getcwd01";
const char *syscall_name = "getcwd";

logging logger;

#define BUF_SIZE 200  // BUF_SIZE must >= MAX_PATH (ie. 128)

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  char buf[BUF_SIZE];
  memset(buf, 0, BUF_SIZE);

  char *p = getcwd(buf, BUF_SIZE);
  // 比较返回的指针和传入的 buf 是否相等
  info(&logger, "getcwd return %d, expected %d\n", p, buf);
  if (p != buf) {
    cleanup();
    exit(TC_FAIL);
  }

  /*
for (int i = 0; i < BUF_SIZE; i++) {
  printf("%d ", buf[i]);
}
  */

  info(&logger, "getcwd return value: %s\n", p);
  info(&logger, "getcwd buf value: %s\n", buf);
  if (strcmp(p, "/") != 0) {
    warning(&logger, "FAILED\n");
    cleanup();
    exit(-1);
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}