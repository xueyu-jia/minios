/*
 * 自己给自己发信号
 *
 */
#include "usertest.h"

const char *test_name = "signal03";
const char *syscall_name = "signal";

logging logger;

const int set_sig = 10;
const int set_arg = 11;
int recved_sig = 0;
int recved_arg = 0;

void sighandler(int sig, uint32_t arg) {
  recved_sig = sig;
  recved_arg = arg;
}

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void run() {
  // 注册信号
  int rval = signal(set_sig, sighandler);
  if (rval == -1) {
    info(&logger, "failed to register signal %d\n", set_sig);
    cleanup();
    exit(-1);
  }

  kill(get_pid(), set_sig, set_arg);
  sleep(10);

  if (recved_sig != set_sig || recved_arg != set_arg) {
    info(&logger,
         "received sig %d, expected %d; received arg %d, expected %d\n",
         test_name, recved_sig, set_sig, recved_arg, set_arg);
    cleanup();
    exit(-1);
  }
  info(&logger, "passed\n");
}

void cleanup() { logger_close(&logger); }

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}