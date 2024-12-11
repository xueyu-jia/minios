/*
 *  测试 pthread_exit 能否正确返回值
 *
 *  1. 创建新线程。通过 pthread_exit 返回一个值
 *  2. 主线程通过调用 pthread_join，接收返回值
 *  3. 比较主线程接收的到返回值和线程的实际返回的值是否一致
 *
 */

#include <usertest.h>

const char *test_name = "pthread_exit01";
const char *syscall_name = "pthread_exit";

logging logger;

const int RETURN_CODE = 100;
static int sem = 0;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void *thread_func(void *arg) {
    sem = 1;
    pthread_exit((void *)RETURN_CODE);
}

void run() {
    pthread_t thread_id;

    // 创建新线程
    int rval = pthread_create(&thread_id, NULL, thread_func, NULL);
    if (rval != 0) {
        error(&logger, "pthread_create return %d, thread_id: %d\n", rval,
              thread_id);
        cleanup();
        exit(TC_FAIL);
    }

    // 确保线程已执行
    while (sem == 0) { sleep(10); }

    // 等待线程结束
    int *value_ptr = 0;
    rval = pthread_join(thread_id, (void *)&value_ptr);
    if (rval != 0) {
        error(&logger, "pthread_join return %d, expected 0\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 比较
    info(&logger, "value_ptr: %d, RETURN_CODE: %d\n", (int)value_ptr,
         RETURN_CODE);
    if ((int)value_ptr != RETURN_CODE) {
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
