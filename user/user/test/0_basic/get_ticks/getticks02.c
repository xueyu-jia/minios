/*
 * 测试 get_ticks 系统调用。
 * get_ticks 应是递增的
 *
 */
#include "usertest.h"

const char *test_name = "getticks02";
const char *syscall_name = "get_ticks";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int ticks1 = get_ticks();
  int ticks2;
  for (int i = 0; i < 20; i++) {
    sleep(i);
    ticks2 = get_ticks();
    if (ticks1 + i > ticks2) {
      info(&logger, "ticks1: %d, ticks2: %d, ticks1 + %d should <= ticks2\n",
           ticks1, ticks2, i);
      cleanup();
      exit(TC_FAIL);
    }
    ticks1 = ticks2;
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
