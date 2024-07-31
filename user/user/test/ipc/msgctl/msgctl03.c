/*
 * 测试 msgctl IPC_RMID 能否删除队列
 *
 */
#include "usertest.h"

const char *test_name = "msgctl03";
const char *syscall_name = "msgctl";

logging logger;

int mq_id;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  // 创建消息队列
  key_t key = ftok("msgctl03_key", 123);
  mq_id = msgget(key, IPC_CREAT | IPC_EXCL);
  if (mq_id < 0) {
    info(&logger, "msgget return %d, expected >= 0\n", mq_id);
    cleanup();
    exit(-1);
  }
}

void cleanup() {
  if (mq_id >= 0) {
    msgctl(mq_id, IPC_RMID, NULL);
  }

  logger_close(&logger);
}

void run() {
  int rval = msgctl(mq_id, IPC_RMID, NULL);
  info(&logger, "msgctl(IPC_RMID) return %d, expected 0\n", rval);
  if (rval != 0) {
    cleanup();
    exit(-1);
  }

  msqid_ds mq_info;
  rval = msgctl(mq_id, IPC_STAT, &mq_info);
  if (rval == 0) {
    info(&logger, "msgctl(IPS_STAT) return %d, expected 0\n", rval);
    cleanup();
    exit(-1);
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}