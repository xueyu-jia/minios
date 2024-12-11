/*
 * 测试 shmctl 能否正确获取共享内存信息
 */
#include <usertest.h>

const char *test_name = "shmctl01";
const char *syscall_name = "shmctl";

logging logger;

int shm_id;
key_t key;
const int SHM_SIZE = 64;
#define SHM_ADDR ShareLinBase
char *shm_addr;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
    key = ftok(test_name, 4123);
}

void cleanup() {
    if (shmctl(shm_id, DELETE, NULL) == -1) {
        info(&logger, "failed to DELETE %d\n", shm_id);
    }

    logger_close(&logger);
}

void run() {
    shm_id = shmget(key, SHM_SIZE, SHM_IPC_CREAT);
    info(&logger, "shmget return %d\n", shm_id);
    if (shm_id == -1) {
        cleanup();
        exit(-1);
    }

    struct ipc_shm shm_info;
    // info(&logger, "size: %d, in_use: %d\n", shm_info.perms_size,
    // shm_info.in_use);
    int rval = shmctl(shm_id, INFO, &shm_info);
    info(&logger, "before _shmat: shmctl return %d, expected %d\n", rval, 0);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }
    shm_addr = _shmat(shm_id, NULL, 0);
    rval = shmctl(shm_id, INFO, &shm_info);
    info(&logger, "after _shmat: shmctl return %d, expected %d\n", rval, 0);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }
    info(&logger, "size: %d, in_use: %d\n", shm_info.perms_size,
         shm_info.in_use);

    _shmdt(shm_addr);
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
