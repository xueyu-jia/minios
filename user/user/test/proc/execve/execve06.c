/*
 * 测试使用 ``execve`` 后，其父进程是否还能接收到其退出返回的状态。
 *
 * 1. fork 多个子进程
 * 2. 各个子进程内 execve 加载 `child06`
 * 3. `child06` 通过 exit 返回一个值
 * 4. 父进程检查返回值是否正确
 *
 */

#include "usertest.h"

const char *test_name = "execve06";
const char *syscall_name = "execve";

logging logger;

#define N_FORKS 20
const char *child_name = "child06";
const int exp_val = 6;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void do_child() {
  int ret = execve(child_name, NULL, NULL);
  // execve 失败，返回 -1
  error(&logger, "failed to execve, return %d\n", ret);
  exit(-1);
}

void run() {
  info(&logger, "fork...\n");
  for (int i = 0; i < N_FORKS; i++) {
    int pid = SAFE_FORK();
    if (pid == 0) {
      do_child();
    }
  }

  info(&logger, "waiting children...\n");
  for (int i = 0; i < N_FORKS; i++) {
    int status;
    wait(&status);
    if (status == -1) { // execve error
      error(&logger, "execve error\n");
      cleanup();
      exit(TC_FAIL);
    } else if (status != exp_val) {
      error(&logger, "child exit %d, expected %d\n", status, exp_val);
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
  exit(TC_FAIL);
}
