/*
 * msgget 在不指定 IPC_CREAT 且消息队列不存在时，应返回 -6(ENOENT)
 *
 */
#include "usertest.h"

const char *test_name = "msgget02";
const char *syscall_name = "msgget";

logging logger;

int mq_id;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  key_t key = ftok("msgget02_key", 23);
  mq_id = msgget(key, 0);
  if (mq_id != ENOENT) {
    info(&logger, "msgget return %d, expected %d\n", mq_id, ENOENT);
    cleanup();
    exit(TC_FAIL);
  }

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
