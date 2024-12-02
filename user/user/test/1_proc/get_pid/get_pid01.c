/*
 * 测试 get_pid 能否能正确返回。miniOS 中 pid 值的范围在 [0, 64) 之间
 * （由 proc.h 中的 NR_PCBS 限制）。
 */

#include <usertest.h>

const char *test_name = "get_pid01";
const char *syscall_name = "get_pid";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int pid = get_pid();
  info(&logger, "pid: %d\n", pid);
  if (pid < 0 || pid >= 64) {
    info(&logger, "pid not in range [0, 64)\n");
    cleanup();
    exit(TC_FAIL);
  }
  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
