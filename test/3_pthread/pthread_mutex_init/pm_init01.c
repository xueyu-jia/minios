/*
 * 测试 pthread_mutex_init 能否正确初始化
 *
 * 通过判断返回值是否为 0 判断是否初始化成功。
 *
 * 1. 测试带 mutexattr 参数的情况
 * 2. 测试 mutexattr 为 NULL 的情况
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_init01";
const char *syscall_name = "pthread_mutex_init";

logging logger;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    pthread_mutexattr_t mta = {
        .name = "pm_init01",
    };
    pthread_mutex_t mutex1, mutex2;
    int rval;

    // 初始化 mutex1，mutexattr 为 mta
    rval = pthread_mutex_init(&mutex1, &mta);
    if (rval != 0) {
        error(&logger, "pthread_mutex_init failed, return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }
    if (strcmp(mutex1.name, "pm_init01") != 0) {
        error(&logger, "pthread_mutex_init failed, return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 初始化 mutex2，mutexattr 为 NULL
    if ((rval = pthread_mutex_init(&mutex1, NULL)) != 0) {
        info(&logger, "pthread_mutex_init failed, return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
