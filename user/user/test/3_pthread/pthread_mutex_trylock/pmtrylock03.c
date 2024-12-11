/*
 * 多次调用 pthread_mutex_trylock，检查操作是否成功
 *
 * 1. 初始化一个 mutex
 * 2. 调用 pthread_mutex_trylock 获取该 mutex，应成功
 * 3. 再次调用 pthread_mutex_trylock 获取该 mutex，应失败
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_trylock02";
const char *syscall_name = "pthread_mutex_trylock";

logging logger;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    int rval;

    rval = pthread_mutex_trylock(&mutex);
    info(&logger, "pthread_mutex_trylock return %d, expected 0\n", rval);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    rval = pthread_mutex_trylock(&mutex);
    info(&logger, "pthread_mutex_trylock return %d, expected -1\n", rval);
    if (rval != -1) {
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
