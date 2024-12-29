/*
 * 测试 shmmemcpy 能否复制数据到指定的位置
 *
 */
#include <usertest.h>

const char *test_name = "shmmemcpy01";
const char *syscall_name = "shmmemcpy";

logging logger;

#define SHM_SIZE 4096
#define SHM_ADDR_1 ShareLinBase
#define SHM_ADDR_2 (ShareLinBase + SHM_SIZE)

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

int get_shm(int proj_id) {
    key_t key = ftok(test_name, proj_id);
    int shm_id = shmget(key, SHM_SIZE, SHM_IPC_CREAT);
    if (shm_id == -1) {
        error(&logger, "failed to create share memory\n");
        cleanup();
        exit(TC_FAIL);
    }

    return shm_id;
}

void *attach_shm(int shm_id, void *shm_addr) {
    void *addr = _shmat(shm_id, shm_addr, 0);
    if (addr == (void *)-1) {
        error(&logger, "failed to attach share memory, shmat return %d\n", addr);
        cleanup();
        exit(TC_UNRESOLVED);
    }

    return addr;
}

void delete_shm(int shm_id, void *addr) {
    _shmdt(addr);

    if (shmctl(shm_id, DELETE, NULL) == -1) {
        error(&logger, "failed to DELETE %d\n", shm_id);
        cleanup();
        exit(TC_UNRESOLVED);
    }
}

// 测试：栈 -> 共享内存
void tc1() {
    info(&logger, "copy from stack to share memory\n");
    int shm_id = get_shm(123);
    void *addr = attach_shm(shm_id, SHM_ADDR_1);

    char *str = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    shmmemcpy(addr, str, strlen(str) + 1);

    if (strcmp(addr, str) != 0) {
        error(&logger, "FAILED\n");
        cleanup();
        exit(TC_FAIL);
    }

    delete_shm(shm_id, addr);
}

// 测试：共享内存 -> 共享内存
void tc2() {
    info(&logger, "copy from share memory to share memory\n");
    int shm_id1 = get_shm(223);
    int shm_id2 = get_shm(225);
    void *addr_1 = attach_shm(shm_id1, SHM_ADDR_1);
    void *addr_2 = attach_shm(shm_id2, SHM_ADDR_2);

    char *str = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    memcpy(addr_1, str, strlen(str) + 1);

    shmmemcpy(addr_2, addr_1, strlen(str) + 1);

    if (strcmp(addr_1, addr_2) != 0) {
        error(&logger, "FAILED\n");
        cleanup();
        exit(TC_FAIL);
    }

    delete_shm(shm_id1, addr_1);
    delete_shm(shm_id2, addr_2);
}

void run() {
    tc1();
    tc2();

    info(&logger, "PASSED\n");
}

int main() {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
