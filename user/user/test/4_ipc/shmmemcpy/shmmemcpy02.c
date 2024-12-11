/*
 * 测试 shmmemcpy 能否正确处理源地址和目的地址存在交叠的情况。
 *
 */
#include <usertest.h>

const char *test_name = "shmmemcpy02";
const char *syscall_name = "shmmemcpy";

logging logger;

#define SHM_SIZE 4096
#define SHM_ADDR ShareLinBase

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    key_t key = ftok(test_name, 133);
    int shm_id = shmget(key, SHM_SIZE, SHM_IPC_CREAT);
    info(&logger, "shmget return %d\n", shm_id);
    if (shm_id == -1) {
        cleanup();
        exit(TC_FAIL);
    }

    void *addr = _shmat(shm_id, SHM_ADDR, 0);
    if (addr == (void *)-1) {
        info(&logger, "failed to attach share memory on %d\n", SHM_ADDR);
        cleanup();
        exit(TC_UNRESOLVED);
    }

    char *str = "0123456789";
    shmmemcpy(addr, str, strlen(str) + 1);

    shmmemcpy(addr + 3, addr, strlen(str) + 1);
    char *exp_str = "0120123456789";

    info(&logger, "get: %s, expected: %s\n", addr, exp_str);
    if (strcmp(addr, exp_str) != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    _shmdt(addr);
    if (shmctl(shm_id, DELETE, NULL) == -1) {
        error(&logger, "failed to DELETE %d\n", shm_id);
        cleanup();
        exit(TC_UNRESOLVED);
    }

    info(&logger, "PASSED\n");
}

int main() {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
