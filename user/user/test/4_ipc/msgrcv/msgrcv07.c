/*
 * 测试 msgrcv 能否读取到指定类型的消息
 *
 * msgtyp:
 * - 为 0 时返回第一条消息
 * - 大于 0 时，返回第一条类型为 msgtyp 的消息。如果没有该类型的消息，则会阻塞，
 *   直到有该类型的消息到达；如果指定了 IPC_NOWAIT，则会立即返回 -1。
 *   如果没有该类型的消息且指定了 MSG_EXCEPT (MiniOS 中未实现)，则返回队列中
 *   的第一条消息
 * - 小于 0 时，返回消息队列中消息类型比 msgtyp 绝对值小且消息类型值最小的
 *   第一条消息。如果没有类型比 msgtype 绝对值小的消息，则会阻塞；如果指定了
 *   IPC_NOWAIT，则会返回 -1
 *
 */
#include <usertest.h>

const char *test_name = "msgrcv07";
const char *syscall_name = "msgrcv";

logging logger;

const long MSG_TYPE_1 = 5;
const long MSG_TYPE_2 = 10;
struct msgbuf recv_buf;
struct msgbuf send_buf_1 = {MSG_TYPE_1, "msgrcv07-test-str-1"};
struct msgbuf send_buf_2 = {MSG_TYPE_2, "msgrcv07-test-str-2"};
int mq_id;
int before_msgrcv, after_msgrcv;

long abs(long num) {
    if (num >= 0) return num;

    return 0 - num;
}

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    // 创建消息队列
    key_t key = ftok("msgrcv07_key", 123);
    mq_id = msgget(key, IPC_CREAT);
    if (mq_id < 0) {
        info(&logger, "msgget return %d, expected an valid id\n", mq_id);
        cleanup();
        exit(-1);
    }

    // 发送消息
    int rval =
        msgsnd(mq_id, &send_buf_1, strlen(send_buf_1.mtext) + 1, IPC_NOWAIT);
    if (rval == -1) {
        info(&logger, "msgsnd return %d, expected 0\n", rval);
        cleanup();
        exit(-1);
    }
    rval = msgsnd(mq_id, &send_buf_2, strlen(send_buf_2.mtext) + 1, IPC_NOWAIT);
    if (rval == -1) {
        info(&logger, "msgsnd return %d, expected 0\n", rval);
        cleanup();
        exit(-1);
    }
}

void cleanup() {
    if (mq_id >= 0) { msgctl(mq_id, IPC_RMID, NULL); }
    logger_close(&logger);
}

// msgtyp > 0 且队列中无此类型的消息
void recv_1() {
    int msgtyp = 20;
    int read_len = msgrcv(mq_id, &recv_buf, MSG_SIZE, msgtyp, IPC_NOWAIT);
    info(&logger, "msgrcv(msgtyp=%d) return %d, expected -1\n", msgtyp,
         read_len);
    if (read_len != -1) {
        cleanup();
        exit(-1);
    }
}

// msgtyp < 0 且队列中无类型小于 msgtype 绝对值的消息
void recv_2() {
    int msgtyp = -2;
    int read_len = msgrcv(mq_id, &recv_buf, MSG_SIZE, msgtyp, IPC_NOWAIT);
    info(&logger, "msgrcv(msgtyp=%d) return %d, expected -1\n", msgtyp,
         read_len);
    if (read_len != -1) {
        cleanup();
        exit(-1);
    }
}

// msgtyp < 0 且队列中存在类型小于 msgtype 绝对值的消息
void recv_3() {
    int msgtyp = -7;
    int mtext_len = strlen(send_buf_1.mtext) + 1;
    before_msgrcv = get_ticks();
    int read_len = msgrcv(mq_id, &recv_buf, MSG_SIZE, msgtyp, IPC_NOWAIT);
    after_msgrcv = get_ticks();
    if (read_len == -1) {
        info(&logger, "msgrcv(msgtyp=%d) return %d, read error\n", msgtyp,
             read_len);
        cleanup();
        exit(-1);
    }

    // 消息类型是否小于 abs(msgtyp)
    info(&logger, "recv_buf.mtype: %d, expected <= %d\n", recv_buf.mtype,
         abs(msgtyp));
    if (recv_buf.mtype > abs(msgtyp)) {
        cleanup();
        exit(-1);
    }

    // 读取到的数据长度是否与写入的数据长度一致
    info(&logger, "msgrcv(msgtype=%d) return %d, expected %d\n", msgtyp,
         read_len, mtext_len);
    if (read_len != mtext_len) {
        cleanup();
        exit(-1);
    }

    // 读取到的数据是否与写入的数据一致
    info(&logger, "recv %s, expected %s\n", recv_buf.mtext, send_buf_1.mtext);
    if (strcmp(recv_buf.mtext, send_buf_1.mtext) != 0) {
        cleanup();
        exit(-1);
    }
}

void run() {
    recv_1();
    recv_2();
    recv_3();

    info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
