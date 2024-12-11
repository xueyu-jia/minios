/*
 * pthread_mutex_unlock 一个未初始化的 mutex 应返回 0
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_unlock03";
const char *syscall_name = "pthread_mutex_unlock";

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

    rval = pthread_mutex_unlock(&mutex);
    info(&logger, "pthread_mutex_unlock return %d, expected %d\n", rval, 0);
    if (rval != 0) {
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
