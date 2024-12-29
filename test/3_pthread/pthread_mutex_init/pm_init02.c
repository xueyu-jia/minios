/*
 * 测试 pthread_mutex_init 初始化后的状态是否为未上锁的状态
 *
 * 1. pthread_mutex_init 初始化一个 mutex
 * 2. 获取该 mutex
 * 3. 释放该 mutex
 * 4. 程序正常结束则说明测试成功
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_init02";
const char *syscall_name = "pthread_mutex_init";

logging logger;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    pthread_mutex_t mutex;
    int rval;

    // 初始化 mutex
    if ((rval = pthread_mutex_init(&mutex, NULL)) != 0) {
        info(&logger, "pthread_mutex_init failed, return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 获取 mutex
    if ((rval = pthread_mutex_lock(&mutex)) != 0) {
        info(&logger, "pthread_mutex_lock failed, return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    sleep(10);

    // 释放 mutex
    if ((rval = pthread_mutex_unlock(&mutex)) != 0) {
        info(&logger, "pthread_mutex_unlock failed, return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 销毁 mutex
    if ((rval = pthread_mutex_destroy(&mutex)) != 0) {
        info(&logger, "pthread_mutex_destroy failed, return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
