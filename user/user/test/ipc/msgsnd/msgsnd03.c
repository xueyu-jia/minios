/*
 * mq_id 无效时，msgsnd 应返回 -1
 *
 */
#include "usertest.h"

const char *test_name = "msgsnd03";
const char *syscall_name = "msgsnd";

logging logger;

const int bad_mq_id = -1;
const long MSG_TYPE = 1;
struct msgbuf send_buf = {MSG_TYPE, "msgsnd03-data"};

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int rval = msgsnd(bad_mq_id, &send_buf, MSG_SIZE, IPC_NOWAIT);
  info(&logger, "msgsnd return %d, expected -1\n", rval);
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