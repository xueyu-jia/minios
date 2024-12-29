/*
 * msgsz 小于 0 时，msgsnd 应返回 -1
 *
 */
#include <usertest.h>

const char *test_name = "msgsnd05";
const char *syscall_name = "msgsnd";

logging logger;

const long MSG_TYPE = 1;
struct msgbuf send_buf = {MSG_TYPE, "msgsnd03-data"};
int mq_id;
int bad_size = -1;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    key_t key = ftok("msgsnd05_key", 4);
    mq_id = msgget(key, IPC_CREAT);
    if (mq_id < 0) {
        info(&logger, "msgget return %d, expected a valid id\n", mq_id);
        exit(-1);
    }
}

void cleanup() {
    if (mq_id >= 0) { msgctl(mq_id, IPC_RMID, NULL); }

    logger_close(&logger);
}

void run() {
    int rval = msgsnd(mq_id, &send_buf, bad_size, IPC_NOWAIT);
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
