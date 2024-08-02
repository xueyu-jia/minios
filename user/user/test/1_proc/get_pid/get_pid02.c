/*
 * 测试 fork 后父进程拿到的 pid 值与子进程 get_pid 的值是否一致。
 *
 * fork 后子进程通过 exit 向父进程返回其 pid 值。父进程比较 fork 得到的 pid
 * 值和 wait 得到的 pid 值
 *
 */

#include "usertest.h"

const char *test_name = "get_pid02";
const char *syscall_name = "get_pid";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int pid = SAFE_FORK();
  if (pid == 0) {    // child process
    exit(get_pid()); // 返回 pid
  }

  // father process
  int status = -1;
  wait(&status);
  info(&logger, "child get_pid: %d, parent fork(): %d\n", status, pid);
  if (status != pid) {
    warning(&logger, "FAILED\n");
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