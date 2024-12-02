/*
 * 测试 malloc_4k 能否申请一块 4k 大小的内存
 *
 * 1. 使用 malloc_4k 申请一块内存
 * 2. 对该内存写入数据，如果所有字节都能写入，则成功。否则会报 page fault
 */
#include <usertest.h>

const char *test_name = "malloc01";
const char *syscall_name = "malloc";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  char *buf = (char *)malloc_4k();
  // 如果没有申请到足够的内存，则报 page fualt
  for (int i = 0; i < 4096; i++) {
    buf[i] = 'a';
  }

  free_4k(buf);

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
