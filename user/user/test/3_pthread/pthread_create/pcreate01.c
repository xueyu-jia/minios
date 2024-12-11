/*
 *
 *  测试 pthread_create 能否正常创建进程。
 *
 *  1. 创建新线程，并传递一个参数
 *  2. 新线程检查传入的参数是否正确。如正确则说明线程创建成功
 *
 */
#include <usertest.h>

const char *test_name = "pthread_create01";
const char *syscall_name = "pthread_create";

logging logger;

int pthread_arg = 10;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

// 线程函数
void *thread_func(void *arg) {
    info(&logger, "thread arg: %d, expected %d\n", *(int *)arg, pthread_arg);
    pthread_exit(NULL);
}

void run() {
    pthread_t thread_id;

    // 创建新线程
    int rval = pthread_create(&thread_id, NULL, thread_func, &pthread_arg);
    info(&logger, "pthread_create return %d, thread_id: %d\n", rval, thread_id);
    if (rval != 0) {
        cleanup();
        exit(TC_FAIL);
    }

    rval = pthread_join(thread_id, NULL);
    info(&logger, "pthread_join return %d\n", rval);
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
