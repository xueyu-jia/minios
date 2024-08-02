/*
 * 测试 msgctl 会失败的几种情况
 * - mq_id 无效
 * - msqid_ds 结构体无效
 * - cmd 无效
 *
 */
#include "usertest.h"

const char *test_name = "msgctl04";
const char *syscall_name = "msgctl";

logging logger;

int mq_id;
int bad_mq_id = -1;
int bad_cmd = -1;
msqid_ds *bad_mq_info_p = (msqid_ds *)-1;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  // 创建消息队列
  key_t key = ftok("msgctl04_key", 123);
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

void run_tc(int mq_id, int cmd, msqid_ds *msqid_ds_p) {
  int rval = msgctl(mq_id, cmd, msqid_ds_p);
  info(&logger,
       "msgctl(mq_id=%d, cmd=%d, msqid_ds=%p) return %d, expected != 0\n",
       test_name, mq_id, cmd, msqid_ds_p, rval);
  if (rval == 0) {
    cleanup();
    exit(-1);
  }
}

void run() {
  msqid_ds mq_info;

  // mq_id 无效
  run_tc(bad_mq_id, IPC_STAT, &mq_info);
  run_tc(bad_mq_id, IPC_SET, &mq_info);

  // cmd 无效
  run_tc(mq_id, bad_cmd, &mq_info);

  // msqid_ds 结构体无效
  run_tc(mq_id, IPC_STAT, bad_mq_info_p);
  run_tc(mq_id, IPC_SET, bad_mq_info_p);

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
