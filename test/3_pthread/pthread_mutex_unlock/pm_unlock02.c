/*
 * 测试不同线程间 pthread_mutex_unlock 能正确释放 mutex
 *
 * 1. 初始化一个全局 mutex
 * 2. 主线程使用 pthread_mutex_lock 获取该 mutex
 * 3. 创建新线程，新线程中使用 pthread_mutex_unlock 释放该 mutex，并结束
 * 4. 主线程使用 pthread_mutex_trylock 尝试获取该 mutex。若
 *    pthread_mutex_trylock 能获取到该 mutex （返回 0），则测试成功。
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_unlock02";
const char *syscall_name = "pthread_mutex_unlock";

logging logger;

static volatile pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void *thread_func() {
    int rval = pthread_mutex_unlock(&mutex);
    if (rval != 0) { info(&logger, "thread: pthread_mutex_unlock: %d\n", rval); }
    pthread_exit(NULL);
}

void run() {
    int rval;
    pthread_t thread_id;
    pthread_t tid;

    // 主线程获取 mutex
    rval = pthread_mutex_lock(&mutex);
    if (rval != 0) {
        info(&logger, "pthread_mutex_lock return %d, expected 0\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 创建线程
    rval = pthread_create(&tid, NULL, thread_func, NULL);
    if (rval != 0) {
        info(&logger, "pthread_create return %d, expected 0\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    sleep(100);

    rval = pthread_mutex_trylock(&mutex);
    if (rval != 0) {
        info(&logger, "pthread_mutex_unlock failed to release mutex");
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
