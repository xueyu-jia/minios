/*
 * 测试 msgctl IPC_SET 能否能修改信息。目前只支持修改 msg_qbytes
 *
 */
#include "usertest.h"

const char *test_name = "msgctl02";
const char *syscall_name = "msgctl";

logging logger;

int mq_id;
msqid_ds ori_mq_info;
msqid_ds mq_info;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  // 创建消息队列
  key_t key = ftok("msgctl02_key", 123);
  mq_id = msgget(key, IPC_CREAT | IPC_EXCL);
  if (mq_id < 0) {
    info(&logger, "msgget return %d, expected >= 0\n", mq_id);
    cleanup();
    exit(-1);
  }

  // 获取队列原来的信息
  int rval = msgctl(mq_id, IPC_STAT, &ori_mq_info);
  if (rval != 0) {
    info(&logger, "ori msgctl(IPC_STAT) return %d, expected 0\n", rval);
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
  msqid_ds mq_info = ori_mq_info;
  mq_info.msg_qbytes -= 1;

  // 设置 msg_qbytes 值
  int rval = msgctl(mq_id, IPC_SET, &mq_info);
  if (rval != 0) {
    info(&logger, "msgctl(IPC_SET) return %d, expected 0\n", rval);
    cleanup();
    exit(-1);
  }

  memset(&mq_info, 0, sizeof(mq_info));
  rval = msgctl(mq_id, IPC_STAT, &mq_info);
  if (rval != 0) {
    info(&logger, "msgctl(IPS_STAT) return %d, expected 0\n", rval);
    cleanup();
    exit(-1);
  }

  // 检查队列的 msq_qbytes 值
  info(&logger, "mq_info.msg_qbytes: %d, expected %d\n", mq_info.msg_qbytes,
       ori_mq_info.msg_qbytes - 1);
  if (mq_info.msg_qbytes != ori_mq_info.msg_qbytes - 1) {
    cleanup();
    exit(-1);
  }

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
