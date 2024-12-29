/*
 *  创建一个消息队列，写入一个消息，然后读取该消息
 *
 */
#include <usertest.h>

const char *test_name = "msgget01";
const char *syscall_name = "msgget";

logging logger;

const long MSG_TYPE = 1;

struct msgbuf recv_buf, send_buf = {MSG_TYPE, "msgget-test-str"};
int mq_id;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    if (mq_id >= 0) { msgctl(mq_id, IPC_RMID, NULL); }
    logger_close(&logger);
}

void send() {
    int rval = msgsnd(mq_id, &send_buf, strlen(send_buf.mtext) + 1, IPC_NOWAIT);
    if (rval == -1) {
        error(&logger, "msgsnd return %d, expected 0\n", rval);
        cleanup();
        exit(TC_UNRESOLVED);
    }
}

void recv() {
    int mtext_len = strlen(send_buf.mtext) + 1;
    // msgtype 为 0, 取出队列中的第一个消息
    int read_len = msgrcv(mq_id, &recv_buf, MSG_SIZE, 0,
                          IPC_NOWAIT); // 取出队列的第一个消息
    info(&logger, "msgrcv return %d\n", read_len);
    if (read_len == -1) {
        error(&logger, "msgrcv return %d, read error\n", read_len);
        cleanup();
        exit(-1);
    }
    // 读取到的数据长度是否与写入的数据长度一致
    if (read_len != mtext_len) {
        error(&logger, "msgrcv return %d, read error\n", read_len);
        cleanup();
        exit(-1);
    }
    info(&logger, "recv %s, expected %s\n", recv_buf.mtext, send_buf.mtext);
    // 读取到的数据是否与写入的数据一致
    if (strcmp(recv_buf.mtext, send_buf.mtext) != 0) {
        cleanup();
        exit(-1);
    }
    info(&logger, "PASSED\n");
}

void run() {
    key_t key = ftok("msg_key", 23);
    mq_id = msgget(key, IPC_CREAT);
    if (mq_id < 0) {
        error(&logger, "msgget return %d, expected an valid id\n", mq_id);
        cleanup();
        exit(TC_UNRESOLVED);
    }

    send();
    recv();
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
