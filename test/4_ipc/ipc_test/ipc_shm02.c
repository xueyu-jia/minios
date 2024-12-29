/*
 *  通过共享内存实现，多线程
 *
 *  A：发送
 *  B：接收
 *
 */
#include <usertest.h>

const char *test_name = "ipc_shm01";
const char *syscall_name = "ipc_shm";

logging logger;

#define SENDER_NUM 2
#define RECEIVER_NUM 2
const char *send_text =
    "line 1\nline 2\nline 3\nline 4\nline 5\n"
    "line 6\nline 7\nline 8\nline 9\nline 10\n";
//"line 11\nline 12\nline 13\nline 14\nline 15\n"
//"line 16\nline 17\nline 18\nline 19\nline 20\n";
const char *send_file = "sendshm.txt";
const char *recv_file = "recvshm.txt";
int send_fd;
int recv_fd;

#define SHM_SIZE 4096
#define BUF_SIZE 2048

struct msg {
    pthread_mutex_t mutex;
    int filled;
    int finished;
    int size;
    int message[BUF_SIZE];
};

key_t key;
int shm_id;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
    info(&logger, "starting\n");

    // 创建 sender.txt
    info(&logger, "prepare send.txt\n");
    int fd = open(send_file, O_CREAT | O_RDWR);
    if (fd < 0) {
        error(&logger, "failed to create send.txt\n");
        cleanup();
        exit(TC_UNRESOLVED);
    }
    write(fd, send_text, strlen(send_text));
    close(fd);

    info(&logger, "setup done\n");
}

void cleanup() {
    close(send_fd);
    close(recv_fd);
    unlink(send_file);

    logger_close(&logger);
}

// 从 fd 中读取一行数据写入到 read_buf 中，读取完后会在 buf 后加 '\0'。不包含
// '\n'。返回读取到的行的长度。
int readline(int fd, char *read_buf, int buf_len) {
    char c = '\0';
    int count = 0;
    int reach_eof = 0;
    while (1) {
        c = '\0';
        read(fd, &c, sizeof(c));
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

void *attach() {
    void *shm_addr = (void *)_shmat(shm_id, NULL, 0);
    if (shm_addr < 0) {
        error(&logger, "process[%d]: failed to attach share memory\n", get_pid());
        exit(-1);
    }
    return shm_addr;
}

void send_message(struct msg *msg_p, char *content, int size) {
    int rval;
    while (1) {
        rval = pthread_mutex_lock(&(msg_p->mutex));
        if (rval != 0) {
            error(&logger, "sender[%d]: failed to lock mutex\n", get_pid());
            exit(-1);
        }
        // check if message consumed
        if (msg_p->filled) {
            pthread_mutex_unlock(&(msg_p->mutex));
            continue;
        }

        memcpy(msg_p->message, content, size);
        msg_p->size = size;
        msg_p->filled = 1;
        pthread_mutex_unlock(&(msg_p->mutex));
        break;
    }
}

void sender_func() {
    int pid = get_pid();
    int rc;
    int rval;
    void *shm_addr;
    struct msg *msg_p;
    char line[BUF_SIZE];
    memset(line, 0, BUF_SIZE);
    char content[BUF_SIZE];
    memset(content, 0, BUF_SIZE);

    info(&logger, "sender[%d]: attach share memory...\n", pid);
    shm_addr = attach();
    msg_p = (struct msg *)shm_addr;

    for (int i = 0;; i++) {
        rc = readline(send_fd, line, BUF_SIZE);
        printf("%d: %s\n", pid, line);
        sleep(100);
        if (rc == 0) { break; }
        rval = sprintf(content, "sender[%d]: %d: %s", pid, i, line);
        send_message(msg_p, content, rval);
    }

    pthread_mutex_lock(&(msg_p->mutex));
    msg_p->finished = 1;
    pthread_mutex_unlock(&(msg_p->mutex));

    info(&logger, "sender[%d]: deattach share memory\n", pid);
    _shmdt(shm_addr);

    info(&logger, "sender[%d] exiting...\n", pid);
    exit(0);
}

void receive_message(struct msg *msg_p) {
    int pid = get_pid();
    char read_buf[BUF_SIZE];

    while (1) {
        pthread_mutex_lock(&(msg_p->mutex));
        if (msg_p->filled) {
            memcpy(read_buf, msg_p->message, msg_p->size);
            read_buf[msg_p->size] = '\0';
            printf("receiver[%d]: %s\n", pid, read_buf);
            sleep(100);
            msg_p->filled = 0;
        }
        if (msg_p->finished) {
            pthread_mutex_unlock(&(msg_p->mutex));
            break;
        }
        pthread_mutex_unlock(&(msg_p->mutex));
    }
}

void receiver_func() {
    int pid = get_pid();
    int rc;
    int rval;

    info(&logger, "receiver[%d]: attach share memory...\n", get_pid());
    void *shm_addr = (void *)attach();
    struct msg *msg_p = (struct msg *)shm_addr;

    receive_message(msg_p);

    info(&logger, "receiver[%d]: deattach share memory\n", pid);
    _shmdt(shm_addr);

    info(&logger, "receiver[%d] exiting...\n", pid);
    exit(0);
}

void run() {
    int rval;
    void *shm_addr;
    struct msg *msg_p;

    info(&logger, "creating share memory...\n");
    key = ftok("ipc_shm01", 1243);
    shm_id = shmget(key, SHM_SIZE, SHM_IPC_CREAT);
    if (shm_id < 0) {
        error(&logger, "failed to create share memory\n");
        cleanup();
        exit(TC_UNRESOLVED);
    }

    info(&logger, "attach share memory...\n");
    shm_addr = (void *)_shmat(shm_id, NULL, 0);
    if (shm_addr < 0) {
        error(&logger, "failed to attach share memory\n");
        cleanup();
        exit(TC_FAIL);
    }

    msg_p = (struct msg *)shm_addr;
    memset(msg_p, 0, sizeof(msg_p));

    info(&logger, "initializing mutex...\n");
    rval = pthread_mutex_init(&(msg_p->mutex), NULL);
    if (rval != 0) {
        error(&logger, "failed to initialize mutex, return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    send_fd = SAFE_OPEN(send_file, O_RDWR);
    recv_fd = SAFE_OPEN(recv_file, O_CREAT | O_RDWR);

    info(&logger, "fork sender...\n");
    for (int i = 0; i < SENDER_NUM; i++) {
        if (fork() == 0) { sender_func(); }
    }

    info(&logger, "fork receiver...\n");
    for (int i = 0; i < RECEIVER_NUM; i++) {
        if (fork() == 0) { receiver_func(); }
    }

    info(&logger, "waitting for all children...\n");
    for (int i = 0; i < SENDER_NUM + RECEIVER_NUM; i++) {
        int status;
        int pid = wait(&status);
        info(&logger, "process[%d] exited, status: %d\n", pid, status);
    }

    info(&logger, "FINISH\n");
    pthread_mutex_destroy(&(msg_p->mutex));
    info(&logger, "FINISH\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
