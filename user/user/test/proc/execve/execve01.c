/*
 * 测试无 argc 和 envp 参数时 execve 功能是否正常。
 * execve 执行成功时不会返回值。失败时返回 -1
 *
 *  调用 execve，如果成功，则运行 child01，并返回 TC_PASS。
 *  如果出错，则 execve 失败返回 -1
 *
 */

#include "usertest.h"

const char *test_name = "execve01";
const char *syscall_name = "execve";

logging logger;

const char *child_name = "child01";

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int ret = execve(child_name, NULL, NULL); // should not return
  error(&logger, "execve return %d\n", ret);
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_FAIL);
}
