/*
 *  测试 shmdt 在 shmaddr 地址上没有 attach 的共享内存时的行为
 *
 *  在 Linux 会返回 -1，并设置 errno 为 EINVAL。然而 _shmdt 在 MiniOS 中是没有返
 *  值的。因此，程序能正常结束就算测试通过。
 *
 */
#include <usertest.h>

const char *test_name = "shmdt01";
const char *syscall_name = "shmdt";

logging logger;

int shm_id;
const key_t KEY = 12;
const int SIZE = 1024;
char *shm_addr;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void run() {
  _shmdt(shm_addr);

  info(&logger, "PASSED\n");
}

void cleanup() { logger_close(&logger); }

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
