/*
 *   msgrcv 在消息内容大于 msgsz 时且未指定 MSG_NOERROR 时，应返回 -1
 *   msgrcv 在消息内容大于 msgsz 时且指定了 MSG_NOERROR 时，应返回截断的消息内容
 *
 */
#include <usertest.h>

const char *test_name = "msgrcv02";
const char *syscall_name = "msgrcv";

logging logger;

const long MSG_TYPE = 1;
struct msgbuf recv_buf, send_buf = {MSG_TYPE, "msgrcv02-test-str"};
int mq_id;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  // 创建消息队列
  key_t key = ftok("msgrcv02_key", 125);
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
  // 未指定 MSG_NOERROR
  // msgtype 为 0, 取出队列中的第一个消息
  int read_len = msgrcv(mq_id, &recv_buf, MSG_SIZE - 1, 0,
                        IPC_NOWAIT);  // 取出队列的第一个消息
  info(&logger, "msgrcv return %d, expected -1\n", read_len);
  if (read_len != -1) {
    cleanup();
    exit(-1);
  }

  // 指定了 MSG_NOERROR
  read_len = msgrcv(mq_id, &recv_buf, MSG_SIZE - 1, 0,
                    IPC_NOWAIT | MSG_NOERROR);  // 取出队列的第一个消息
  info(&logger, "msgrcv return %d, expected %d\n", read_len, MSG_SIZE - 1);
  if (read_len != MSG_SIZE - 1) {
    cleanup();
    exit(-1);
  }

  // 检查内容是否一致
  for (int i = 0; i < MSG_SIZE - 1; i++) {
    if (send_buf.mtext[i] != recv_buf.mtext[i]) {
      info(&logger, "send_buf.mtext[%d]: %d, recv_buf.mtext[%d]: %d\n",
           test_name, i, send_buf.mtext[i], i, recv_buf.mtext[i]);
      cleanup();
      exit(-1);
    }
  }

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
