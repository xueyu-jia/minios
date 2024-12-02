/*
 * pthread_mutex_lock 一个未初始化的 mutex 应返回 0
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_lock04";
const char *syscall_name = "pthread_mutex_lock";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int rval;
  pthread_mutex_t mutex;

  rval = pthread_mutex_lock(&mutex);
  info(&logger, "pthread_mutex_lock return %d, expected 0\n", rval);
  if (rval != 0) {
    cleanup();
    exit(TC_FAIL);
  }

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
