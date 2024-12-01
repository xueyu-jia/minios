/*
 * 测试 msgrcv 能否读取到预期的消息
 *
 */
#include "usertest.h"

const char *test_name = "msgrcv01";
const char *syscall_name = "msgrcv";

logging logger;

const long MSG_TYPE = 1;
struct msgbuf recv_buf, send_buf = {MSG_TYPE, "msgrcv01-test-str"};
int mq_id;
int before_msgrcv, after_msgrcv;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  // 创建消息队列
  key_t key = ftok("msgrcv01_key", 123);
  mq_id = msgget(key, IPC_CREAT);
  if (mq_id < 0) {
    info(&logger, "msgget return %d, expected an valid id\n", mq_id);
    cleanup();
    exit(-1);
  }

  // 发送消息
  int rval = msgsnd(mq_id, &send_buf, strlen(send_buf.mtext) + 1, IPC_NOWAIT);
  if (rval == -1) {
    info(&logger, "msgsnd return %d, expected 0\n", rval);
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

void recv() {
  int mtext_len = strlen(send_buf.mtext) + 1;
  before_msgrcv = get_ticks();
  // msgtype 为 0, 取出队列中的第一个消息
  int read_len = msgrcv(mq_id, &recv_buf, MSG_SIZE, 0,
                        IPC_NOWAIT);  // 取出队列的第一个消息
  after_msgrcv = get_ticks();
  if (read_len == -1) {
    info(&logger, "msgrcv return %d, read error\n", read_len);
    cleanup();
    exit(-1);
  }

  // 读取到的数据长度是否与写入的数据长度一致
  info(&logger, "msgrcv return %d, expected %d\n", read_len, mtext_len);
  if (read_len != mtext_len) {
    cleanup();
    exit(-1);
  }

  // 读取到的数据是否与写入的数据一致
  info(&logger, "recv %s, expected %s\n", recv_buf.mtext, send_buf.mtext);
  if (strcmp(recv_buf.mtext, send_buf.mtext) != 0) {
    cleanup();
    exit(-1);
  }
}

void check() {
  // 获取队列信息
  msqid_ds mq_info;
  int rval = msgctl(mq_id, IPC_STAT, &mq_info);
  if (rval == -1) {
    info(&logger, "msgctl return %d, expected 0\n", rval);
    cleanup();
    exit(-1);
  }

  // 检查队列中的消息数量
  info(&logger, "mq_info.msg_qnum: %d, expected 0\n", mq_info.msg_qnum);
  if (mq_info.msg_qnum != 0) {  // 消息队列中已无消息
    cleanup();
    exit(-1);
  }

  // 检查队列中的数据长度
  info(&logger, "mq_info.__msg_cbytes: %d, expected 0\n", mq_info.__msg_cbytes);
  if (mq_info.__msg_cbytes != 0) {
    cleanup();
    exit(-1);
  }

  // 检查队列的最后接收时间
  info(&logger, "mq_info.msg_rtime: %d, expected between %d and %d\n",
       test_name, mq_info.msg_rtime, before_msgrcv, after_msgrcv);
  if (mq_info.msg_rtime < before_msgrcv || mq_info.msg_rtime > after_msgrcv) {
    cleanup();
    exit(-1);
  }

  // 检查接收方 pid
  info(&logger, "mq_info.msg_lrpid: %d, expected %d\n", mq_info.msg_lrpid,
       get_pid());
  if (mq_info.msg_lrpid != get_pid()) {
    cleanup();
    exit(-1);
  }
}

void run() {
  recv();
  check();

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}