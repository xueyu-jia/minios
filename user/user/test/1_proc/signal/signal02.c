/*
 *  设置一个无效的信号（sig >= NR_SIGNALS）应返回 -1
 *
 */
#include "usertest.h"

const char *test_name = "signal02";
const char *syscall_name = "signal";

logging logger;

void handler(int sig, uint32_t arg) { printf("handler %d %d\n", sig, arg); }

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void tc(int sig) {
  int rval = signal(sig, handler);
  info(&logger, "signal(%d) return %d, expected %d\n", sig, rval, -1);
  if (rval != -1) {
    cleanup();
    exit(-1);
  }
}

void run() {
  tc(32);
  tc(40);
  tc(-1);
  tc(-100);
  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}