/*
 * pthread_cond_init 初始化一个已经初始化过的条件变量。
 *
 *  // 在 POSIX 标准中，这是一个未定义行为
 *
 */
#include <usertest.h>

const char *test_name = "pthread_cond_init03";
const char *syscall_name = "pthread_cond_init";

logging logger;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    int rval;

    pthread_cond_t cond;
    rval = pthread_cond_init(&cond, NULL);
    info(&logger, "pthread_cond_init return %d, expected 0\n", rval);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    // init again
    rval = pthread_cond_init(&cond, NULL);
    info(&logger, "pthread_cond_init return %d, expected 0\n", rval);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    rval = pthread_cond_destroy(&cond);
    if (rval != 0) {
        error(&logger, "failed to destroy cond!");
        cleanup();
        exit(TC_UNRESOLVED);
    }

    info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
