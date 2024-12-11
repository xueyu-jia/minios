/*
 * 测试申请大于 4k 的内存空间应返回 -1
 *
 */

#include <usertest.h>

const char *test_name = "shmget05";
const char *syscall_name = "shmget";

logging logger;

int shm_id;
const key_t KEY = 12;
const int SIZE = 1024 * 1024;
char *shm_addr;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    shm_id = shmget(KEY, SIZE, SHM_IPC_CREAT);
    info(&logger, "shmget return %d, expected %d\n", shm_id, -1);
    if (shm_id != -1) {
        cleanup();
        exit(TC_FAIL);
    }
    info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
