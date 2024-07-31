/*
 * msgget 在消息队列已满（4, MAX_MSQ_NUM）时，应返回 -8(ENOSPC)
 *
 */
#include "usertest.h"

const char *test_name = "msgget03";
const char *syscall_name = "msgget";

logging logger;

int mq_ids[MAX_MSQ_NUM];

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  for (int i = 0; i < MAX_MSQ_NUM; i++) {
    mq_ids[i] = -1;
    mq_ids[i] = msgget(ftok("msgget03_key", i + 12), IPC_CREAT);
    if (mq_ids[i] == -1) {
      info(&logger, "msgget reutn %d, expected a valid id\n", mq_ids[i]);
      cleanup();
      exit(-1);
    }
  }
}

void cleanup() {
  for (int i = 0; i < MAX_MSQ_NUM; i++) {
    if (mq_ids[i] >= 0) {
      msgctl(mq_ids[i], IPC_RMID, NULL);
    }
  }

  logger_close(&logger);
}

void run() {
  key_t key = ftok("msgget03_key", 23);
  int mq_id = msgget(key, IPC_CREAT);
  if (mq_id != ENOSPC) {
    error(&logger, "msgget return %d, expected %d\n", mq_id, ENOSPC);
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
