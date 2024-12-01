/*
 * 测试 fork 能否正常工作，wait 后返回的子进程 pid 值是否和 fork 返回的 pid
 * 值一致，并检查 exit 返回的 status 值
 *
 * 1. fork，检查 fork 返回值是否小于 0
 * 2. 子进程 exit，父进程检查其返回值是否是其 pid，并检查 status
 *
 */

#include "usertest.h"

const char *test_name = "fork01";
const char *syscall_name = "fork";

logging logger;

#define FORK_NUM 100

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  for (int i = 0; i < FORK_NUM; i++) {
    int pid = fork();
    if (pid < 0) {
      error(&logger, "fork failed\n");
      cleanup();
      exit(TC_FAIL);
    } else if (pid == 0) {  //  child  process
      exit(i);
    }

    // father process
    int status;
    int child_pid = wait(&status);
    info(&logger, "child return pid: %d, returen status: %d\n", child_pid,
         status);
    if (child_pid != pid || status != i) {
      warning(&logger, "FAILED\n");
      cleanup();
      exit(TC_FAIL);  // 测试失败
    }
    info(&logger, "PASSED\n");
  }
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}