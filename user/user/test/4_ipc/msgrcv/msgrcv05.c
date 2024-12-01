/*
 *   msgrcv 在 msgsz 为小于 0 时，应返回 -1
 *
 */
#include "usertest.h"

const char *test_name = "msgrcv05";
const char *syscall_name = "msgrcv";

logging logger;

const long MSG_TYPE = 1;
struct msgbuf recv_buf, send_buf = {MSG_TYPE, "msgrcv05-test-str"};
int mq_id;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  // 创建消息队列
  key_t key = ftok("msgrcv05_key", 127);
  mq_id = msgget(key, IPC_CREAT);
  if (mq_id < 0) {
    info(&logger, "msgget return %d, expected an valid id\n", mq_id);
    cleanup();
    exit(-1);
  }

  // 发送消息
  int rval = msgsnd(mq_id, &send_buf, MSG_SIZE, IPC_NOWAIT);
  if (rval == -1) {
    info(&logger, "msgsnd return %d, expected 0\n", rval);
    cleanup();
    exit(-1);
  }
}

void cleanup() {
  logger_close(&logger);
  if (mq_id >= 0) {
    msgctl(mq_id, IPC_RMID, NULL);
  }
}

void run() {
  // msgtype 为 0, 取出队列中的第一个消息
  // msgsz 小于 0
  int read_len = msgrcv(mq_id, &recv_buf, -1, 0,
                        IPC_NOWAIT);  // 取出队列的第一个消息
  info(&logger, "msgrcv return %d, expected -1\n", read_len);
  if (read_len != -1) {
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