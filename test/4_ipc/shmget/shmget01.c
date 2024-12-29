/*
 *  测试 shmget 是否能正确获取一块共享内存
 *
 */
#include <usertest.h>

const char *test_name = "shmget01";
const char *syscall_name = "shmget";

logging logger;

const int SHM_ADDR_LINE = ShareLinBase;
const int SHM_SIZE = 4096;

int shm_id;
key_t key;
char *shm_addr;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    key = ftok("shmget01", 1234);
}

void run() {
    info(&logger, "shmget...\n");
    shm_id = shmget(key, SHM_SIZE, SHM_IPC_CREAT);
    info(&logger, "shmget return %d\n", shm_id);
    if (shm_id == -1) {
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "shmat\n");
    shm_addr = (char *)_shmat(shm_id, NULL, 0);
    info(&logger, "_shmat return %d\n", shm_addr);
    if (shm_addr == (void *)-1) {
        cleanup();
        exit(TC_FAIL);
    }
    // 对该共享内存写入数据应不报错
    for (int i = 0; i < SHM_SIZE; i++) { shm_addr[i] = 'a'; }
}

void cleanup() {
    _shmdt(shm_addr);
    int rval = shmctl(shm_id, DELETE, NULL);
    if (rval != 0) { error(&logger, "failed to DELETE %d, shmctl return %d\n", shm_id, rval); }
    logger_close(&logger);
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
