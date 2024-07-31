/*
 * msgget 指定了 IPC_CREAT 和 IPC_EXCL 创建消息队列时，若该 key 对应的
 * 消息队列已存在，则应返回 EEXIST
 *
 * EXCL means exclusive
 *
 */
#include "usertest.h"

const char *test_name = "msgget04";
const char *syscall_name = "msgget";

logging logger;

int mq_id;
key_t key;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  key = ftok("msgget04_key", 2341);
  mq_id = msgget(key, IPC_CREAT);
  if (mq_id == -1) {
    info(&logger, "msgget reutn %d, expected a valid id\n", mq_id);
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
  int id = msgget(key, IPC_CREAT | IPC_EXCL);
  if (id != EEXIST) {
    info(&logger, "msgget return %d, expected %d\n", mq_id, EEXIST);
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
