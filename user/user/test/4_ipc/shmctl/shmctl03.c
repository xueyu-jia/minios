/*
 * INFO 查看共享内存信息时，如果传入的 buf 为 NULL，应返回 -1
 *
 */

#include <usertest.h>

const char *test_name = "shmctl03";
const char *syscall_name = "shmctl";

logging logger;

#define SHM_SIZE 4096
key_t shm_key;
int shm_id;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
    shm_key = ftok(test_name, 78);
    shm_id = shmget(shm_key, SHM_SIZE, SHM_IPC_CREAT);
    if (shm_id < 0) {
        error(&logger, "failed to create share memory\n");
        exit(TC_UNRESOLVED);
    }
}

void run() {
    int rval = shmctl(shm_id, INFO, NULL);
    info(&logger, "shmctl return %d, expected: %d", rval, -1);
    if (rval != -1) {
        cleanup();
        exit(TC_FAIL);
    }
}

void cleanup() {
    if (shmctl(shm_id, DELETE, NULL) == -1) {
        error(&logger, "failed to DELETE %d\n", shm_id);
        cleanup();
        exit(TC_UNRESOLVED);
    }

    logger_close(&logger);
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
