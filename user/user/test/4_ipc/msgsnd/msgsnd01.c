/*
 * 测试 msgsnd 是否能正常发送消息。使用 msgctl 获取相关信息来验证。
 *
 */
#include <usertest.h>

const char *test_name = "msgsnd01";
const char *syscall_name = "msgsnd";

logging logger;

int mq_id;
const long MSG_TYPE = 1;
struct msgbuf send_buf = {MSG_TYPE, "msgsnd01-data"};

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    key_t key = ftok("msgsnd01_key", 23);
    mq_id = msgget(key, IPC_CREAT);
    if (mq_id < 0) {
        info(&logger, "msgget return %d, expected a valid id\n", mq_id);
        exit(-1);
    }
}

void cleanup() {
    logger_close(&logger);

    if (mq_id >= 0) { msgctl(mq_id, IPC_RMID, NULL); }
}

void run() {
    int before_msgsnd = get_ticks();
    int rval = msgsnd(mq_id, &send_buf, strlen(send_buf.mtext) + 1, IPC_NOWAIT);
    int after_msgsnd = get_ticks();
    info(&logger, "msgsnd return %d, expected 0\n", rval);
    if (rval == -1) {
        cleanup();
        exit(-1);
    }

    msqid_ds mq_info;
    rval = msgctl(mq_id, IPC_STAT, &mq_info);
    info(&logger, "msgctl return %d, expected 0\n", rval);
    if (rval == -1) {
        cleanup();
        exit(-1);
    }

    info(&logger, "mq_info.msg_qnum: %d, expected 1\n", mq_info.msg_qnum);
    if (mq_info.msg_qnum != 1) { // 消息队列中只有一个消息
        cleanup();
        exit(-1);
    }

    info(&logger, "mq_info.__msg_cbytes: %d, expected %d\n",
         mq_info.__msg_cbytes, strlen(send_buf.mtext) + 1);
    if (mq_info.__msg_cbytes != strlen(send_buf.mtext) + 1) { // 检查数据长度
        cleanup();
        exit(-1);
    }

    // 检查发送时间
    info(&logger, "mq_info.msg_stime: %d, expected between %d and %d\n",
         test_name, mq_info.msg_stime, before_msgsnd, after_msgsnd);
    if (mq_info.msg_stime < before_msgsnd || mq_info.msg_stime > after_msgsnd) {
        cleanup();
        exit(-1);
    }

    // 检查发送 pid
    info(&logger, "mq_info.msg_lspid: %d, expected %d\n", mq_info.msg_lspid,
         get_pid());
    if (mq_info.msg_lspid != get_pid()) {
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
