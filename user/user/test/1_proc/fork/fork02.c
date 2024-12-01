/*
 * 测试系统连续 fork 是否会出错
 *
 * miniOS 中最多只支持 48 个用户进程（由 NR_PCBS 和 NR_K_PCBS 限制）。
 *
 */

#include "usertest.h"

const char *test_name = "fork02";
const char *syscall_name = "fork";

logging logger;

const int fork_count = 40;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  info(&logger, "start fork...\n");
  for (int i = 0; i < fork_count; i++) {
    int pid = fork();
    if (pid < 0) {
      info(&logger, "fork failed\n");
      cleanup();
      exit(TC_FAIL);
    } else if (pid == 0) {
      info(&logger, "child pid: %d\n", get_pid());
      exit(0);
    }
  }

  info(&logger, "waiting for children exit...\n");
  int status;
  for (int i = 0; i < fork_count; i++) {
    wait(&status);
    if (status != 0) {
      info(&logger, "child process not exit 0\n");
      cleanup();
      exit(TC_FAIL);
    }
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}