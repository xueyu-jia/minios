/*
 *  通过消息队列实现，多进程
 *
 *  A：发送
 *  B：接收
 *
 */
#include <usertest.h>

const char *test_name = "ipc_msg02";
const char *syscall_name = "ipc_msg";

logging logger;

#define SENDER_NUM 4
#define RECEIVER_NUM 4
const char *send_text =
    "line 1\nline 2\nline 3\nline 4\nline 5\n"
    "line 6\nline 7\nline 8\nline 9\nline 10\n"
    "line 11\nline 12\nline 13\nline 14\nline 15\n"
    "line 16\nline 17\nline 18\nline 19\nline 20\n";
const char *send_file = "send.txt";
const char *recv_file = "recv.txt";
int send_fd;
int recv_fd;

key_t key;
int mq_id;               // 消息队列 id
const long MSG_TYPE = 1; // 消息类型

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
    info(&logger, "starting\n");

    // 创建 sender.txt
    info(&logger, "prepare send.txt\n");
    int fd = open(send_file, O_CREAT | O_RDWR, I_RW);
    if (fd < 0) {
        error(&logger, "failed to create send.txt\n");
        cleanup();
        exit(TC_UNRESOLVED);
    }
    int text_len = strlen(send_text);
    write(fd, send_text, text_len);
    close(fd);

    info(&logger, "setup done\n");
}

void cleanup() {
    unlink(send_file);

    logger_close(&logger);
    printf("main exit\n");
}

// 从 fd 中读取一行数据写入到 read_buf 中，读取完后会在 buf 后加 '\0'。不包含
// '\n'。返回读取到的行的长度。
int readline(int fd, char *read_buf, int buf_len) {
    int rc;
    char c = '\0';
    int count = 0;
    int reach_eof = 0;
    while (1) {
        c = '\0';
        rc = read(fd, &c, sizeof(c));
        // read 系统调用读取到 EOF 时应返回 0.
        // 但 MiniOS read 调用目前都只会返回传入的 count 值。
        // 这里通过检查是否有数据写入 c 来判断是否已读到 EOF
        if (c == '\0') {
            reach_eof = 1;
            break;
        }
        // 读完一行
        if (c == '\n') { break; }
        read_buf[count++] = c;
    }
    // 当读取到数据或是空行时，在末尾添加 '\0'
    if (count != 0 || !reach_eof) { read_buf[count] = '\0'; }
    return count;
}

void sender_func() {
    int rval;
    int rc;
    int pid = get_pid();
    char message[MSG_SIZE];
    char line[MSG_SIZE];
    memset(line, 0, MSG_SIZE);
    struct msgbuf send_buf;

    // info(&logger, "send_fd: %d\n", send_fd);
    int sent = 1;
    for (int i = 1;; i++) {
        if (sent) {
            // 读取一行
            rc = readline(send_fd, line, MSG_SIZE);
            if (rc == 0) { // 文件已读完
                break;
            }
            sent = 0;
            info(&logger, "sender[%d] readline: %s\n", pid, line);
            // 构造消息
            memset(message, 0, MSG_SIZE);
            rval = sprintf(message, "sender[%d]: %d: %s", pid, i, line);
            send_buf.mtype = MSG_TYPE;
            strncpy(send_buf.mtext, message, rval + 1);
            info(&logger, "sender[%d] send message: %s\n", pid, message);
        }
        // msgsnd 在队列满时不会阻塞。故这里无法通过返回值判断是队列满还是出错。
        rval = msgsnd(mq_id, &message, strlen(send_buf.mtext), 0);
        if (rval == 0) {
            sent = 1;
        } else {
            sleep(100);
        }
    }

    info(&logger, "sender[%d] exiting...\n", pid);
    exit(0);
}

void receiver_func() {
    int rval;
    int rc;
    int pid = get_pid();
    char write_buf[MSG_SIZE];
    struct msgbuf recv_buf;

    // info(&logger, "recv_fd: %d\n", recv_fd);
    while (1) {
        sleep(100 + pid);
        // 取出队列中的第一条消息
        rc = msgrcv(mq_id, &recv_buf, MSG_SIZE, 0, IPC_NOWAIT);
        // 队列中已无消息
        if (rc == -1) { break; }
        info(&logger, "receiver[%d] receive: %s\n", pid, recv_buf.mtext);
        memset(write_buf, 0, MSG_SIZE);
        rval = sprintf(write_buf, "receiver[%d]: %s\n", pid, recv_buf.mtext);
        write(recv_fd, write_buf, rval);
    }

    info(&logger, "receiver[%d] exiting...\n", pid);
    exit(0);
}

void run() {
    int sender_pids[SENDER_NUM];
    int receriver_pids[RECEIVER_NUM];

    info(&logger, "creating message queue...\n");
    key = ftok("ipc_msg", 123);
    mq_id = msgget(key, IPC_CREAT | IPC_EXCL);
    if (mq_id < 0) {
        error(&logger, "failed to create message queue\n");
        cleanup();
        exit(TC_UNRESOLVED);
    }

    send_fd = open(send_file, O_RDWR);
    if (send_fd < 0) {
        error(&logger, "failed to open %s\n", send_file);
        cleanup();
        exit(TC_UNRESOLVED);
    }
    // create senders
    info(&logger, "creating senders...\n");
    for (int i = 0; i < SENDER_NUM; i++) {
        sender_pids[i] = fork();
        if (sender_pids[i] < 0) {
            error(&logger, "failed to fork sender\n");
        } else if (sender_pids[i] == 0) {
            sender_func();
        }
    }

    recv_fd = open(recv_file, O_RDWR | O_CREAT, I_RW);
    if (recv_fd < 0) {
        error(&logger, "failed to open %s\n", recv_file);
        cleanup();
        exit(TC_UNRESOLVED);
    }
    info(&logger, "creating receivers...\n");
    for (int i = 0; i < RECEIVER_NUM; i++) {
        receriver_pids[i] = fork();
        if (receriver_pids[i] < 0) {
            error(&logger, "failed to fork receiver\n");
        } else if (receriver_pids[i] == 0) {
            receiver_func();
        }
    }

    info(&logger, "waiting for all children finish\n");
    for (int i = 0; i < SENDER_NUM + RECEIVER_NUM; i++) { int pid = wait(NULL); }
    info(&logger, "closing fds\n");
    close(send_fd);
    close(recv_fd);
    info(&logger, "closing fds done\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
