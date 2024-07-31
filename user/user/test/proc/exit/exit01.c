/*
 * 测试 exit 能否正确返回状态值和进程的 pid
 *
 */

#include "usertest.h"

const char *test_name = "exit01";
const char *syscall_name = "exit";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  for (int exit_status = -20; exit_status <= 127; exit_status++) {
    int pid = SAFE_FORK();
    if (pid == 0) {
      exit(exit_status);
    } else {
      int status;
      int ret_pid = wait(&status);
      info(&logger, "expected pid: %d, actual return pid %d\n", pid, ret_pid);
      info(&logger, "expected status code: %d, actual status code %d\n",
           exit_status, status);
      if (status != exit_status || pid != ret_pid) {
        warning(&logger, "FAILED\n");
        cleanup();
exit(TC_FAIL);
      }
    }
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}