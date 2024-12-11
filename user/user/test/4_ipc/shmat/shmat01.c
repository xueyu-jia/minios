/*
 * shmat 在指定 addr 为 NULL 时，应自动选择地址
 *
 * 1. 申请一块共享内存
 * 2. 使用 shmat attach 该共享内存，应返回一个地址
 * 3. 再次使用 shmat attach 该共享内存，应返回另外一个地址
 * 4. 这两个地址都指向同一块物理内存
 *
 */
#include <usertest.h>

const char *test_name = "shmat01";
const char *syscall_name = "shmat";

logging logger;

#define SHM_SIZE 4096

key_t key;
int shm_id;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    info(&logger, "creating share memory...\n");
    key = ftok("shmat01", 12243);
    shm_id = shmget(key, SHM_SIZE, SHM_IPC_CREAT);
    if (shm_id < 0) {
        error(&logger, "failed to create share memory, return %d\n", shm_id);
        cleanup();
        exit(TC_UNRESOLVED);
    }
}

void cleanup() {
    int ret = shmctl(shm_id, DELETE, NULL);
    if (ret != 0) {
        warning(&logger, "shmctl delete share memory reutn : %d\n", ret);
    }

    logger_close(&logger);
}

void run() {
    void *addr1 = _shmat(shm_id, NULL, NULL);
    if (addr1 == -1) {
        error(&logger, "shmat return %d, expected a valid address\n", addr1);
        cleanup();
        exit(TC_FAIL);
    }
    void *addr2 = _shmat(shm_id, NULL, NULL);
    if (addr1 == -1) {
        error(&logger, "shmat return %d, expected a valid address\n", addr2);
        cleanup();
        exit(TC_FAIL);
    }

    if (addr1 == addr2) {
        error(&logger, "addr1 equals addr2!\n");
        cleanup();
        exit(TC_FAIL);
    }

    _shmdt(addr1);
    _shmdt(addr2);

    info(&logger, "PASSED\n");
}

int main() {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
