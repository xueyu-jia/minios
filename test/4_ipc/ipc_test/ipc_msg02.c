/*
 *  通过消息队列实现，多线程
 *
 *  A：发送
 *  B：接收
 *
 */
#include <usertest.h>

const char *test_name = "ipc_msg02";
const char *syscall_name = "ipc_msg";

#define SHM_ADDR ShareLinBase

#define SENDER_NUM 2
#define RECEIVER_NUM 3
const char *send_text =
    "line 1\nline 2\nline 3\nline 4\nline 5\n"
    "line 6\nline 7\nline 8\nline 9\nline 10\n"
    "line 11\nline 12\nline 13\nline 14\nline 15\n"
    "line 16\nline 17\nline 18\nline 19\nline 20\n";
const char *send_filename = "send02.txt";
const char *recv_filename = "recv02.txt";
int send_fd;
int recv_fd;

pthread_mutex_t read_lock;
pthread_mutex_t write_lock;

key_t key;
int mq_id;               // 消息队列 id
const long MSG_TYPE = 1; // 消息类型

void setup() {
    int rval;

    // logger_init(&logger, log_filename, test_name, LOG_INFO);
    // info(&logger, "starting\n");
    printf("starting...\n");

    // 创建 sender.txt
    // info(&logger, "prepare send.txt\n");
    printf("prepare send file: %s\n", send_filename);
    int fd = open(send_filename, O_CREAT | O_RDWR);
    if (fd < 0) {
        printf("failed to create %s\n", send_filename);
        // error(&logger, "failed to create send.txt\n");
        cleanup();
        exit(TC_UNRESOLVED);
    }
    int n = write(fd, send_text, strlen(send_text));
    if (n != strlen(send_text)) {
        printf("partial write\n");
        exit(-1);
    }
    close(fd);

    // 初始化锁
    rval = pthread_mutex_init(&read_lock, NULL);
    if (rval != 0) {
        printf("failed to initialize read_lock\n");
        // error(&logger, "failed to initialize read_lock\n");
        cleanup();
        exit(TC_UNRESOLVED);
    }
    rval = pthread_mutex_init(&write_lock, NULL);
    if (rval != 0) {
        // error(&logger, "failed to initialize write_lock\n");
        printf("failed to initialize write_lock\n");
        cleanup();
        exit(TC_UNRESOLVED);
    }

    printf("setup done\n");
    // info(&logger, "setup done\n");
}

void cleanup() {
    printf("cleanup\n");
    if (send_fd > 0) {
        // close(send_fd);
    }
    // unlink(send_filename);

    // logger_close(&logger);
    printf("main exit\n");
}

// 从 fd 中读取一行数据写入到 read_buf 中，读取完后会在 buf 后加 '\0'。不包含
// '\n'。返回读取到的行的长度。
//
// 需要用互斥锁来保证每个线程能完整地读取一行
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
    // if (count != 0 || !reach_eof) {
    read_buf[count] = '\0';
    //}
    return count;
}

void *sender_func() {
    int rval;
    int rc;
    pthread_t tid = pthread_self();
    char message[MSG_SIZE];
    char line[MSG_SIZE];
    memset(line, 0, MSG_SIZE);
    struct msgbuf send_buf;

    int sent = 1;
    for (int i = 1;; i++) {
        if (sent) {
            pthread_mutex_lock(&read_lock);
            // TODO handle error
            // info("sender[%d]: acquire lock\n", pthread_self());
            printf("sender[%d]: acquire lock\n", tid);
            // 读取一行
            rc = readline(send_fd, line, MSG_SIZE);
            pthread_mutex_unlock(&read_lock);
            printf("sender[%d]: release lock\n", tid);
            // info("sender[%d]: release lock\n", pthread_self());
            if (rc == 0) { // 文件已读完
                break;
            }
            sent = 0;
            printf("sender[%d] readline: %s\n", tid, line);
            // info(&logger, "sender[%d] readline: %s\n", tid, line);
            //   构造消息
            //   memset(message, 0, MSG_SIZE);
            rval = sprintf(send_buf.mtext, "sender[%d]: %d: %s", tid, i, line);
            send_buf.mtype = MSG_TYPE;
            // printf("sender[%d]: send_buf.mtext: %s\n", tid, send_buf.mtext);
            sleep(100);
            // strncpy(send_buf.mtext, message, rval + 1);
            // //info(&logger, "sender[%d] send message: %s\n", tid, message);
        }
        // msgsnd 在队列满时不会阻塞。故无法通过返回值判断是队列满还是出错。
        rval = msgsnd(mq_id, &send_buf, strlen(send_buf.mtext), IPC_NOWAIT);
        if (rval == 0) {
            sent = 1;
        } else {
            // 这里视作队列满进行等待
            yield();
            sleep(100);
        }
    }

    printf("sender[%d] exiting...\n", tid);
    // info(&logger, "sender[%d] exiting...\n", tid);
    pthread_exit(NULL);
}

void *receiver_func() {
    int rval;
    int rc;
    int tid = pthread_self();
    char write_buf[MSG_SIZE];
    struct msgbuf recv_buf;

    while (1) {
        sleep(500);
        // 取出队列中的第一条消息
        rc = msgrcv(mq_id, &recv_buf, MSG_SIZE, 0, IPC_NOWAIT);
        // 队列中已无消息
        if (rc == -1) { break; }
        printf("receiver[%d] receive: %s\n", tid, recv_buf.mtext);
        // info(&logger, "receiver[%d] receive: %s\n", tid, recv_buf.mtext);
        memset(write_buf, 0, MSG_SIZE);
        rval = sprintf(write_buf, "receiver[%d]: %s\n", tid, recv_buf.mtext);

        pthread_mutex_lock(&write_lock);
        write(recv_fd, write_buf, rval);
        pthread_mutex_unlock(&write_lock);
    }

    printf("receiver[%d] exiting...\n", tid);
    // info(&logger, "receiver[%d] exiting...\n", tid);
    pthread_exit(NULL);
}

void run() {
    pthread_t sender_tids[SENDER_NUM];
    pthread_t receriver_tids[RECEIVER_NUM];

    printf("creating message queue...\n");
    // info(&logger, "creating message queue...\n");
    key = ftok(test_name, 123);
    mq_id = msgget(key, IPC_CREAT | IPC_EXCL);
    if (mq_id < 0) {
        printf("failed to create message queue\n");
        // error(&logger, "failed to create message queue\n");
        cleanup();
        exit(TC_UNRESOLVED);
    }

    send_fd = open(send_filename, O_RDWR);
    if (send_fd < 0) {
        printf("failed to open %s\n", send_filename);
        // error(&logger, "failed to open %s\n", send_filename);
        cleanup();
        exit(TC_UNRESOLVED);
    }

    // create senders
    printf("creating senders...\n");
    // info(&logger, "creating senders...\n");
    for (int i = 0; i < SENDER_NUM; i++) {
        pthread_create(sender_tids + i, NULL, sender_func, NULL);
    }

    // 创建要写入的文件
    recv_fd = open(recv_filename, O_RDWR | O_CREAT);
    if (recv_fd < 0) {
        printf("failed to open %s\n", recv_filename);
        // error(&logger, "failed to open %s\n", recv_filename);
        cleanup();
        exit(TC_UNRESOLVED);
    }

    printf("creating receivers...\n");
    // info(&logger, "creating receivers...\n");
    for (int i = 0; i < RECEIVER_NUM; i++) {
        pthread_create(receriver_tids + i, NULL, receiver_func, NULL);
    }

    printf("waiting for all threads join...\n");
    // info(&logger, "waiting for all threads join\n");
    for (int i = 0; i < SENDER_NUM; i++) { pthread_join(sender_tids[i], NULL); }
    for (int i = 0; i < RECEIVER_NUM; i++) { pthread_join(receriver_tids[i], NULL); }

    // info(&logger, "closing fds\n");
    close(send_fd);
    close(recv_fd);
    printf("closing fds done\n");
    // info(&logger, "closing fds done\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
