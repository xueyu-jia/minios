/*
 * 测试宏 PTHREAD_COND_INITIALIZER 能否初始化条件变量
 *
 *  PTHREAD_COND_INITIALIZER 只用于初始化静态变量
 */
#include <usertest.h>

const char *test_name = "pthread_cond_init02";
const char *syscall_name = "pthread_cond_init";

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int rval;
  rval = pthread_cond_destroy(&cond);
  if (rval != 0) {
    error(&logger, "failed to destroy cond\n");
    cleanup();
    exit(TC_FAIL);
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
