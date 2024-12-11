/*
 * 测试 pthread_self 返回的 tid 值与 pthread_create 中得到的 tid 值是否一致
 *
 * 1. 创建一个新线程，pthrea_create 返回的线程 id 保存到 th_id1
 * 2. 新线程调用 pthread_self 并将结果保存到一个全局变量 th_id2
 * 3. 比较 th_id1 和 th_id2 的值，应相等
 * 4. 比较主线程 id 和 th_id1 的值，应不相等
 *
 */
#include <usertest.h>

const char *test_name = "pthread_self01";
const char *syscall_name = "pthread_self";

logging logger;

static pthread_t th_id2;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

// 线程函数
void *thread_func(void *arg) {
    th_id2 = pthread_self();
    info(&logger, "pthrea_self: %d\n", th_id2);
    info(&logger, "pid: %d\n", get_pid());
    pthread_exit(NULL);
}

void run() {
    pthread_t th_id1;

    // 创建新线程
    info(&logger, "creating thread...\n");
    int rval = pthread_create(&th_id1, NULL, thread_func, NULL);
    if (rval != 0) {
        info(&logger, "pthread_create return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "before join: %d\n", th_id1);

    rval = pthread_join(th_id1, NULL);
    if (rval != 0) {
        info(&logger, "pthread_join(%d) return %d\n", th_id1, rval);
        cleanup();
        exit(TC_FAIL);
    }

    // th_id1 和 th_id2 应相等。 th_id1 和主线程 id 应该不相等
    info(&logger, "main thread id: %d, th_id1: %d, th_id2: %d\n",
         pthread_self(), th_id1, th_id2);
    if (th_id1 != th_id2 || th_id1 == pthread_self()) {
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
