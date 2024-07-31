/*
 * 测试 msgctl IPC_STAT 能否读取到正确信息
 *
 */
#include "usertest.h"

const char *test_name = "msgctl01";
const char *syscall_name = "msgctl";

logging logger;

int mq_id;
int before_msgget;
int after_msgget;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  // 创建消息队列
  key_t key = ftok("msgctl01_key", 123);
  before_msgget = get_ticks();
  mq_id = msgget(key, IPC_CREAT);
  after_msgget = get_ticks();
  if (mq_id < 0) {
    info(&logger, "msgget return %d, expected an valid id\n", mq_id);
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
  // 获取队列信息
  msqid_ds mq_info;
  memset(&mq_info, 'a', sizeof(mq_info));

  int rval = msgctl(mq_id, IPC_STAT, &mq_info);
  if (rval == -1) {
    info(&logger, "msgctl return %d, expected 0\n", rval);
    cleanup();
    exit(-1);
  }

  // 检查队列中的消息数量
  info(&logger, "mq_info.msg_qnum: %d, expected 0\n", mq_info.msg_qnum);
  if (mq_info.msg_qnum != 0) { // 消息队列中无消息
    cleanup();
    exit(-1);
  }

  // 检查队列中的数据长度
  info(&logger, "mq_info.__msg_cbytes: %d, expected 0\n", mq_info.__msg_cbytes);
  if (mq_info.__msg_cbytes != 0) {
    cleanup();
    exit(-1);
  }

  // 检查队列的最后发送时间
  info(&logger, "mq_info.msg_stime: %d, expected 0\n", mq_info.msg_stime);
  if (mq_info.msg_stime != 0) {
    cleanup();
    exit(-1);
  }

  // 检查队列的最后接收时间
  info(&logger, "mq_info.msg_rtime: %d, expected 0\n", mq_info.msg_rtime);
  if (mq_info.msg_rtime != 0) {
    cleanup();
    exit(-1);
  }

  // 检查队列的最后修改时间
  info(&logger, "mq_info.msg_ctime: %d, expected between %d and %d\n",
       test_name, mq_info.msg_ctime, before_msgget, after_msgget);
  /*
if (mq_info.msg_ctime > after_msgget || mq_info.msg_ctime < before_msgget) {
  cleanup();
  exit(-1);
}
  */

  // 检查最后发送 pid
  info(&logger, "mq_info.msg_lspid: %d, expected %d\n", mq_info.msg_lspid, 0);
  if (mq_info.msg_lspid != 0) {
    cleanup();
    exit(-1);
  }

  // 检查最后接收 pid
  info(&logger, "mq_info.msg_lrpid: %d, expected %d\n", mq_info.msg_lrpid, 0);
  if (mq_info.msg_lrpid != 0) {
    cleanup();
    exit(-1);
  }

  // 检查队列可用空间大小
  info(&logger, "mq_info.msg_qbytes: %d, expected > %d\n", mq_info.msg_qbytes,
       0);
  if (mq_info.msg_qbytes <= 0) {
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
