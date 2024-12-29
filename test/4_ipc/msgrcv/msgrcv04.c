/*
 *   msgrcv 在队列 id 无效时，应返回 -1
 *
 */
#include <usertest.h>

const char *test_name = "msgrcv04";
const char *syscall_name = "msgrcv";

logging logger;

const long MSG_TYPE = 1;
struct msgbuf recv_buf, send_buf = {MSG_TYPE, "msgrcv04-test-str"};
int bad_mq_id;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    // msgtype 为 0, 取出队列中的第一个消息
    int read_len = msgrcv(bad_mq_id, &recv_buf, MSG_SIZE, 0,
                          IPC_NOWAIT); // 取出队列的第一个消息
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
