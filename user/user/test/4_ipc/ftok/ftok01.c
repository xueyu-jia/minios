/*
 * 检查 ftok  的返回值。 ftok 成功时应返回 key 值，失败时应返回 -1.
 *
 */
#include "usertest.h"

const char *test_name = "ftok01";
const char *syscall_name = "ftok";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  key_t key = ftok("ftok01.key", 23);
  info(&logger, "key returned by ftok: %d\n", key);
  if (key == -1) {
    warning(&logger, "failed to ftok! return %d\n", key);
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