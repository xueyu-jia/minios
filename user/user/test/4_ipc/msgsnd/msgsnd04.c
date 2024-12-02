/*
 * posix 标准中，msgbuf 中 mtype 必须为非 0 的正数。如果不是，msgsnd 应返回 -1
 *
 */
#include <usertest.h>

const char *test_name = "msgsnd04";
const char *syscall_name = "msgsnd";

logging logger;

int mq_id;
struct msgbuf send_buf1 = {0, "msgsnd04-data-1"};
struct msgbuf send_buf2 = {-1, "msgsnd04-data-2"};

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  key_t key = ftok("msgsnd04_key", 4);
  mq_id = msgget(key, IPC_CREAT);
  if (mq_id < 0) {
    info(&logger, "msgget return %d, expected a valid id\n", mq_id);
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
  int rval = msgsnd(mq_id, &send_buf1, MSG_SIZE, IPC_NOWAIT);
  info(&logger, "msgsnd send_buf1 return %d, expected -1\n", rval);
  if (rval != -1) {
    cleanup();
    exit(-1);
  }

  rval = msgsnd(mq_id, &send_buf2, MSG_SIZE, IPC_NOWAIT);
  info(&logger, "msgsnd send_buf2 return %d, expected -1\n", rval);
  if (rval != -1) {
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
