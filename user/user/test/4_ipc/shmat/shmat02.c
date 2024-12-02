/*
 *  shmat 一个不存在的 shmid 时，应失败返回 -1
 *
 */
#include <usertest.h>

const char *test_name = "shmat02";
const char *syscall_name = "shmat";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int bad_shm_id = -134;
  int rval = _shmat(bad_shm_id, NULL, NULL);
  info(&logger, "shmat return %d\n", rval);
  if (rval != -1) {
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
