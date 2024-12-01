/*
 * 应返回大于 0 的整数
 */

#include "usertest.h"

const char *test_name = "totalmemsz";
const char *syscall_name = "total_mem_size";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  unsigned int sz1 = total_mem_size();
  void *p = malloc_4k();
  unsigned int sz2 = total_mem_size();
  free_4k(p);
  unsigned int sz3 = total_mem_size();

  info(&logger, "sz1: %x, sz2: %x, sz3: %x\n", sz1, sz2, sz3);
  if (sz1 == sz2 || sz1 <= 0 || sz2 <= 0 || sz3 <= 0) {
    error(&logger, "failed\n");
    cleanup();
    exit(-1);
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
