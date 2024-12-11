/*
 * 被 pthread_mutex_destroy 销毁的 mutex 应能被 pthread_mutex_init 重新初始化。
 *
 * 1. 初始化一个带参数的 mutex
 * 2. 使用 pthread_mutex_destroy 销毁该 mutex
 * 3. 用另外的一个参数再初始化该 mutex
 * 4. 检查再次初始化的 mutex 参数是否正确
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_destroy02";
const char *syscall_name = "pthread_mutex_destroy";

logging logger;

static pthread_mutex_t mutex;

void setup() {
    int rval;

    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    int rval;
    logger_close(&logger);

    rval = pthread_mutex_destroy(&mutex);
    if (rval != 0) {
        info(&logger, "pthread_mutex_destroy mutex return %d, expected 0\n",
             test_name, rval);
        cleanup();
        exit(TC_FAIL);
    }
}

void run() {
    int rval;
    pthread_mutexattr_t mutex_attr1 = {
        .name = "pthread_mutex_destroy02 old mutex attr name",
    };
    pthread_mutexattr_t mutex_attr2 = {
        .name = "pthread_mutex_destroy02 new mutex attr name",
    };

    rval = pthread_mutex_init(&mutex, &mutex_attr1);
    if (rval != 0) {
        info(&logger, "pthread_mutex_init mutex failed, return %d\n", rval);
        cleanup();
        exit(TC_UNRESOLVED);
    }

    rval = pthread_mutex_destroy(&mutex);
    info(&logger, "pthread_mutex_destroy mutex return %d, expected 0\n",
         test_name, rval);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    rval = pthread_mutex_init(&mutex, &mutex_attr2);
    info(&logger, "pthread_mutex_init mutex return %d, expected 0\n", rval);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "mutex.name: %s, expected: %s\n", mutex.name,
         mutex_attr2.name);
    if (strcmp(mutex.name, mutex_attr2.name) != 0) {
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
