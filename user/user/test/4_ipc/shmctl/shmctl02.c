/*
 * 删除一个不存在的共享内存应返回 1
 */

#include <usertest.h>

const char *test_name = "shmctl02";
const char *syscall_name = "shmctl";

logging logger;

int bad_shm_id = -1;
int shm_id;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void run() {
  int rval = shmctl(shm_id, DELETE, NULL);
  info(&logger, "shmctl return %d, expected: %d", rval, -1);
  if (rval != -1) {
    cleanup();
    exit(TC_FAIL);
  }
}

void cleanup() { logger_close(&logger); }

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
