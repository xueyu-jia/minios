/*
 * 测试 get_ticks 系统调用。
 *
 */
#include <usertest.h>

const char *test_name = "getticks01";
const char *syscall_name = "get_ticks";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int ticks = get_ticks();
  info(&logger, "ticks: %d\n", ticks);
  if (ticks <= 0) {
    error(&logger, "ticks should >= 0\n");
    error(&logger, "failed\n");
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
