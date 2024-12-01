/*
 *  测试 MiniOS 是否是并发的。
 *
 * 1. 父进程创建子进程，子进程 sleep(1000)
 * 2. 父进程 sleep(1000)
 * 3. 父进程调用 wait 等待子进程结束
 * 4. 父进程检查运行的时间，应远小于 2000
 *
 */

#include "usertest.h"

const char *test_name = "fork06";
const char *syscall_name = "fork";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

int run() {
  info(&logger, "fork child...\n");
  int start_time = get_ticks();
  if (fork() == 0) {
    info(&logger, "child sleep...\n");
    sleep(1000);
    exit(0);
  }

  info(&logger, "parent sleep...\n");
  sleep(1000);
  wait(NULL);
  int elapsed_time = get_ticks() - start_time;
  info(&logger, "elapsed_time: %d, expected << 2000\n", elapsed_time);
  if (elapsed_time > 2000) {
    error(&logger, "FAILED\n");
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