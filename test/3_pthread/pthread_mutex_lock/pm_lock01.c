/*
 * 测试 pthread_mutex_lock 是否能正确
 *
 * 1. 初始化 mutex，初始化 value 为 0
 * 2. 创建 THREAD_NUM 个线程，每个线程循环 LOOP_N 次对 value 加 1
 * 3. 所有线程结束后，主线程检查 value 的值是否为 THREAD_NUM * LOOP_N。
 *    如果是，则测试成功
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_lock01";
const char *syscall_name = "pthread_mutex_lock";

logging logger;

#define THREAD_NUM 15
#define LOOP_N 30

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int value = 0; // 由 mutex 保护

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void *thread_func(void *arg) {
    int rval;
    pthread_t tid = pthread_self();
    for (int i = 0; i < LOOP_N; i++) {
        // 获取 mutex
        rval = pthread_mutex_lock(&mutex);
        if (rval != 0) { info(&logger, "thread %d: failed to lock mutex, return %d\n", tid, rval); }
        // 对全局变量 value + 1
        value++;
        // 释放 mutex
        rval = pthread_mutex_unlock(&mutex);
        if (rval != 0) {
            info(&logger, "thread %d: failed to unlock mutex, return %d\n", test_name, tid, rval);
        }
    }
    pthread_exit(NULL);
}

void run() {
    int rval;
    pthread_t tids[THREAD_NUM];

    // 创建线程
    for (int i = 0; i < THREAD_NUM; i++) {
        rval = pthread_create(tids + i, NULL, thread_func, NULL);
        if (rval != 0) {
            info(&logger, "main thread: pthread_create return %d\n", rval);
            cleanup();
            exit(TC_FAIL);
        }
    }

    // 等待所有线程结束
    for (int i = 0; i < THREAD_NUM; i++) {
        rval = pthread_join(tids[i], NULL);
        if (rval != 0) {
            info(&logger, "main thread: pthread_join(%d) return %d\n", tids[i], rval);
            cleanup();
            exit(TC_FAIL);
        }
    }

    info(&logger, "value: %d, expected: %d\n", value, THREAD_NUM * LOOP_N);
    if (value != THREAD_NUM * LOOP_N) {
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
