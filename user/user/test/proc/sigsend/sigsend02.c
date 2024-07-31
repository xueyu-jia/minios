/*
 * - pid 不存在时 sigsend 应返回 -1
 * - signal 无效时应返回 -1
 *
 */
#include "usertest.h"

const char *test_name = "sigsend02";
const char *syscall_name = "sigsend";

logging logger;

int sig_num = 1;

void handler(int sig, uint32_t arg) { printf("handler %d %d\n", sig, arg); }

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  signal(sig_num, handler);
}

void run() {
  Sigaction sigaction1 = {.sig = 55, .handler = NULL, .arg = 1};
  int pid = get_pid();
  int rval = sigsend(pid, &sigaction1);
  info(&logger, "invalid sig: sigsend return %d, expected %d\n", rval, -1);
  if (rval != -1) {
    cleanup();
    exit(-1);
  }

  int invalid_pid = 122;
  Sigaction sigaction2 = {.sig = sig_num, .handler = NULL, .arg = 1};
  rval = sigsend(invalid_pid, &sigaction2);
  info(&logger, "invalid pid: sigsend return %d, expected %d\n", rval, -1);
  if (rval != -1) {
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