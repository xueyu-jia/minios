/*
 * 无子进程时，调用 wait 应返回 -1
 *
 */

#include <usertest.h>

const char *test_name = "wait01";
const char *syscall_name = "wait";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int start_time = get_ticks();
  int ret = wait(NULL);
  int elapsed_time = get_ticks() - start_time;
  info(&logger, "wait return %d, expected %d\n", ret, -1);
  if (ret != -1) {
    warning(&logger, "FAILED\n");
    cleanup();
    exit(TC_FAIL);
  }
  info(&logger, "wait elapsed_time: %d\n", elapsed_time);
  if (elapsed_time >= 5) {
    warning(&logger, "FAILED\n");
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
