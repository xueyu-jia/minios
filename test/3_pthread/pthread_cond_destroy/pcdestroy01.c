/*
 * pthread_cond_destroy 应能销毁条件变量 cond，使其变回未初始化的状态。
 *
 * 1. 初始化 3 个条件变量，分别对应使用宏、带参数、不带参数的情况
 * 2. 使用 pthread_cond_destroy 销毁这 3 个条件变量，检查返回值是否为 0
 *
 */
#include <usertest.h>

const char *test_name = "pthread_cond_destroy01";
const char *syscall_name = "pthread_cond_destroy";

logging logger;

pthread_cond_t cond1, cond2;
static pthread_cond_t cond3 = PTHREAD_COND_INITIALIZER;

void setup() {
    int rval;
    logger_init(&logger, log_filename, test_name, LOG_INFO);
    info(&logger, "\nstarting\n");

    pthread_condattr_t condattr = {"cond name"};
    rval = pthread_cond_init(&cond1, &condattr);
    if (rval != 0) {
        error(&logger, "pthread_cond_init cond1 failed, return %d\n", rval);
        cleanup();
        exit(TC_UNRESOLVED);
    }

    rval = pthread_cond_init(&cond2, NULL);
    if (rval != 0) {
        error(&logger, "pthread_cond_init cond2 failed, return %d\n", rval);
        cleanup();
        exit(TC_UNRESOLVED);
    }
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    int rval;

    rval = pthread_cond_destroy(&cond1);
    info(&logger, "pthread_cond_destroy cond1 return %d, expected 0\n", rval);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    rval = pthread_cond_destroy(&cond2);
    info(&logger, "pthread_cond_destroy cond2 return %d, expected 0\n", rval);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    rval = pthread_cond_destroy(&cond3);
    info(&logger, "pthread_cond_destroy cond3 return %d, expected 0\n", rval);
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
