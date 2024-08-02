/*
 * 测试 sleep 是否能正常工作
 *
 * sleep 的输入参数为 ticks
 *
 */

#include "usertest.h"

const char *test_name = "sleep01";
const char *syscall_name = "sleep";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  for (int i = 10; i < 100; i++) {
    int start = get_ticks();
    sleep(i);
    int end = get_ticks();
    int ticks = end - start;
    if (ticks < i) { // 经过的 ticks 应该不小于设定的 ticks
      info(&logger, "end - start = %d, expected >= %d\n", ticks, i);
      exit(-1);
    }
  }
  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}