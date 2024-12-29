/*
 *  测试进程间能否使用共享内存通信
 *
 * 1. 父进程使用 shm_key 创建一块共享内存，写入数据
 * 2. 父进程 fork，等待子进程结束
 * 3. 子进程使用 shm_key
 * 获取该共享内存，并从中读取数据，检查数据是否与预期相同。
 *    如不相同则结束进程返回 -1
 * 4. 子进程向共享内存中写入数据，并结束进程返回状态值 0
 * 5. 父进程检查子进程结束的状态值，若不为 0，则说明子进程中的测试失败
 * 6. 父进程从共享内存中读取数据，检查数据是否与预期相同。
 *
 */

#include <usertest.h>

const char *test_name = "shmget02";
const char *syscall_name = "shmget";

logging logger;

int shm_id;
key_t shm_key;
const int SIZE = 64;
char *shm_addr;

const char *p_str = "parent send to child";
const char *c_str = "child send to parent";

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
    shm_key = ftok(test_name, 3228);
}

void child() {
    int shmid = shmget(shm_key, SIZE, SHM_IPC_FIND);
    info(&logger, "child: shmget return %d\n", shmid);
    if (shmid == -1) { exit(-1); }

    // shm_addr = (char *)_shmat(shm_id, ShareLinBase, 0);
    shm_addr = (char *)_shmat(shm_id, NULL, 0);
    info(&logger, "child: shmat return %d\n", shm_addr);
    if (shm_addr == (void *)-1) { exit(-1); }

    info(&logger, "child received: %s, expected: %s\n", (char *)shm_addr, p_str);
    // 检查接收到的数据与预期是否相符
    if (strcmp((char *)shm_addr, p_str) != 0) {
        memcpy(shm_addr, c_str, strlen(c_str) + 1);
        _shmdt(shm_addr);
        exit(-1);
    }

    // 写入数据
    memcpy(shm_addr, c_str, strlen(c_str) + 1);
    _shmdt(shm_addr);
    exit(0);
}

void run() {
    info(&logger, "creating share memory...\n");
    shm_id = shmget(shm_key, SIZE, SHM_IPC_CREAT);
    info(&logger, "shmget return %d\n", shm_id);
    if (shm_id == -1) {
        cleanup();
        exit(TC_FAIL);
    }

    shm_addr = (char *)_shmat(shm_id, NULL, 0);
    info(&logger, "parent: shmat return %d\n", shm_addr);
    if (shm_addr == (void *)-1) {
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "parent: write data\n");
    memcpy(shm_addr, p_str, strlen(p_str) + 1);

    info(&logger, "fork...\n");
    int pid = SAFE_FORK();
    if (pid == 0) {
        // child process
        child();
    }

    int status;
    wait(&status);
    info(&logger, "child return %d, expected %d\n", status, 0);
    if (status != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "parent received: %s, expected: %s\n", (char *)shm_addr, c_str);
    if (strcmp((char *)shm_addr, c_str) != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "PASSED\n");
}

void cleanup() {
    _shmdt(shm_addr);
    if (shmctl(shm_id, DELETE, NULL) == -1) { error(&logger, "failed to DELETE %d\n", shm_id); }

    logger_close(&logger);
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
