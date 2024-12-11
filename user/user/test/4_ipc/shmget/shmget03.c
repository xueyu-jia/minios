/*
 *  测试是否能申请所有的共享内存
 *
 *  1. 申请完所有的共享内存（数量由 SHM_SIZE 控制），都应能申请成功
 *  2. 再次申请共享内存，应失败返回 -1
 *
 */

#include <usertest.h>

const char *test_name = "shmget03";
const char *syscall_name = "shmget";

logging logger;

int shm_id;
const key_t KEY = 12;
const int SHM_SIZE = 64;
#define NR_SHM 10 // 10 or 256
char *shm_addr;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void run() {
    info(&logger, "shmget...\n");
    int shm_id[NR_SHM];
    for (int i = 0; i < NR_SHM; i++) {
        shm_id[i] = shmget(KEY + i, SHM_SIZE, SHM_IPC_CREAT);
        info(&logger, "%d: shmget return %d\n", i, shm_id[i]);
        if (shm_id[i] == -1) {
            info(&logger, "shmget %d: return %d\n", i, shm_id[i]);
            cleanup();
            exit(TC_FAIL);
        }
    }

    // this should failed
    int fail_id = shmget(KEY + 121, SHM_SIZE, SHM_IPC_CREAT);
    info(&logger, "shmget return %d, expected %d\n", fail_id, -1);
    if (fail_id != -1) {
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "delete share memory...\n");
    for (int i = 0; i < NR_SHM; i++) {
        int rval = shmctl(shm_id[i], DELETE, NULL);
        if (rval != 0) {
            error(&logger, "%d: failed to delete share memory, return %d\n", i,
                  rval);
        }
    }
}

void cleanup() {
    logger_close(&logger);
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
