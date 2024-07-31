/*
 * 测试 IPC_FIND 模式下，shmget 能否返回 key 对应的共享内存
 *
 * 1. 申请一块共享内存，得到共享内存的 shm_id
 * 2. 使用相同的 key，并设置 IPC_FIND flag，shmget 应返回与 shm_id 相同的值
 * 3. 使用不同的 key，并设置 IPC_FIND flag，shmget 应返回 -1 表示失败
 */

#include "usertest.h"

const char *test_name = "shmget04";
const char *syscall_name = "shmget";

logging logger;

int shm_id;
const key_t KEY = 12;
const key_t fail_key = 20;
const int SHM_SIZE = 64;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  shm_id = shmget(KEY, SHM_SIZE, SHM_IPC_CREAT);
  info(&logger, "shmget return %d\n", shm_id);
  if (shm_id == -1) {
    cleanup();
    exit(TC_FAIL);
  }
}

void run() {
  int shm_id_2 = shmget(KEY, SHM_SIZE, SHM_IPC_FIND);
  info(&logger, "shmget return %d, expected %d\n", shm_id_2, shm_id);
  if (shm_id_2 != shm_id) {
    cleanup();
    exit(TC_FAIL);
  }

  int fail_id = shmget(fail_key, SHM_SIZE, SHM_IPC_FIND);
  info(&logger, "shmget return %d, expected %d\n", fail_id, -1);
  if (fail_id != -1) {
    cleanup();
    exit(TC_FAIL);
  }
}

void cleanup() {
  if (shmctl(shm_id, DELETE, NULL) == -1) {
    error(&logger, "failed to DELETE %d\n", shm_id);
  }
  logger_close(&logger);
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
