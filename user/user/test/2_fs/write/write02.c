/*
 * 当使用 write 写入特殊值 NULL 且 size 为 0 时，write 应返回 0
 *
 */

#include <usertest.h>

const char *test_name = "write02";
const char *syscall_name = "write";

logging logger;

const char *filename = "write02.txt";
int fd;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
}

void cleanup() {
  if (fd > 0) {
    SAFE_CLOSE(fd);
    unlink(filename);
  }
  logger_close(&logger);
}

void run() {
  int n = write(fd, NULL, 0);
  info(&logger, "write return %d, expected %d\n", n, 0);
  if (n != 0) {
    error(&logger, "write failed\n");
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
