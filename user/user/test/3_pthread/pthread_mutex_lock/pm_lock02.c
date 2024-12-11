/*
 * 测试 pthread_mutex_lock 成功时的返回值是否为 0
 *
 * 1. 初始化一个 mutex
 * 2. 使用 pthread_mutex_lock 获取该 mutex。检查其返回值是否为 0
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_lock02";
const char *syscall_name = "pthread_mutex_lock";

logging logger;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    int rval;
    pthread_mutex_t mutex;

    rval = pthread_mutex_init(&mutex, NULL);
    if (rval != 0) {
        info(&logger, "pthread_mutex_init failed, return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 获取 mutex
    rval = pthread_mutex_lock(&mutex);
    info(&logger, "pthread_mutex_lock return %d, expected 0\n", rval);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    pthread_mutex_unlock(&mutex);
    info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
