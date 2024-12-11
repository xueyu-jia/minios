/*
 * pthread_mutex_trylock 一个未被初始化的 mutex 应返回 0
 *
 * 1. 初始化一个 mutex
 * 2. 调用 pthread_mutex_trylock 获取该 mutex，应返回 0
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_trylock04";
const char *syscall_name = "pthread_mutex_trylock";

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

    rval = pthread_mutex_trylock(&mutex);
    info(&logger, "pthread_mutex_trylock return %d, expected 0\n", rval);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
