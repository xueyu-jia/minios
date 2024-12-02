/*
 * 测试以下情况
 *
 * getcwd 应返回 NULL：
 *  1. buf 指向一个无效地址
 *  2. size 非法（size < 0）
 *  3. size 为 0 或 1 时
 *  4. buf 为 NULL 且 size 为 1
 *
 */

#include <usertest.h>

const char *test_name = "getcwd02";
const char *syscall_name = "getcwd";

logging logger;

#define BUF_SIZE 200  // BUF_SIZE must >= MAX_PATH (ie. 128)

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

// buf 指向一个无效地址
void test_1() {
  char *p = getcwd((void *)-1, 0);
  if (p != NULL) {
    error(&logger, "getcwd return %d, expected NULL\n", p);
    cleanup();
    exit(TC_FAIL);
  }
  info(&logger, "test_1 passed\n");
}

// size 非法（size < 0）
void test_2() {
  char read_buf[BUF_SIZE];
  memset(read_buf, 0, BUF_SIZE);
  char *p = getcwd(read_buf, -1);
  if (p != NULL) {
    error(&logger, "getcwd return %d, expected NULL\n", p);
    cleanup();
    exit(TC_FAIL);
  }
  info(&logger, "test_2 passed\n");
}

// size 为 0 或 1 时
void test_3() {
  char read_buf[BUF_SIZE];
  memset(read_buf, 0, BUF_SIZE);
  char *p = getcwd(read_buf, 0);
  if (p != NULL) {
    error(&logger, "getcwd return %d, expected NULL\n", p);
    cleanup();
    exit(TC_FAIL);
  }
  p = getcwd(read_buf, 1);
  if (p != NULL) {
    error(&logger, "getcwd return %d, expected NULL\n", p);
    cleanup();
    exit(TC_FAIL);
  }
  info(&logger, "test_3 passed\n");
}

// buf 为 NULL 且 size 为 1
void test_4() {
  char *p = getcwd(NULL, 1);
  if (p != NULL) {
    error(&logger, "getcwd return %d, expected NULL\n", p);
    cleanup();
    exit(TC_FAIL);
  }
  info(&logger, "test_4 passed\n");
}

void run() {
  test_1();
  test_2();
  test_3();
  test_4();

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
