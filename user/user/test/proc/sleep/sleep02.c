/*
 * 测试 sleep 传入的 ticks 为负数时能否正常工作
 *
 *
 */

#include "usertest.h"

const char *test_name = "sleep02";
const char *syscall_name = "sleep";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int start = get_ticks();
  sleep(-1);
  int end = get_ticks();
  int ticks = end - start;
  if (ticks < 0) { // 经过的 ticks 应该不小于 0
    info(&logger, "end - start = %d, expected >= 0\n", ticks);
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