/*
 * 测试 pthread_cond_wait 等待的线程被唤醒后能否正确的重新获取 mutex
 *
 * 1. 创建新线程
 * 2. 主线程等待新线程运行到 pthread_cond_wait
 * 3. 此时主线程应能获取到 mutex
 * 4. 主线程设置 signaled 0 1 并唤醒新线程。新线程检查 pthread_cond_wait
 *    是被阻塞 （signaled 为 1）； signaled 为 0 则说明线程没有被正确阻塞
 * 5. 新线程 trylock mutex 应失败返回 -1。trylock 成功则说明 pthread_cond_wait
 *    未正确获取 mutex
 * 6. 程序正常结束并返回 TC_PASS 说明测试成功
 *
 *
 */
#include <usertest.h>

const char *test_name = "pthread_cond_wait02";
const char *syscall_name = "pthread_cond_wait";

logging logger;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int signaled = 0;
int started = 0;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
    info(&logger, "starting\n");
}

void cleanup() {
    int rval;

    rval = pthread_mutex_destroy(&mutex);
    if (rval != 0) {
        error(&logger, "failed to destroy mutex! return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    rval = pthread_cond_destroy(&cond);
    if (rval != 0) {
        error(&logger, "failed to destroy cond! return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    logger_close(&logger);
}

// 线程函数
void *thread_func(void *arg) {
    int rval;
    pthread_t tid = pthread_self();

    // 必须先持有 mutex
    info(&logger, "thread[%d]: acquire mutex\n");
    rval = pthread_mutex_lock(&mutex);
    if (rval != 0) {
        error(&logger, "thread[%d]: failed to acquire mutex, return %d\n", tid,
              rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 线程已运行
    started = 1;
    info(&logger, "thread[%d]: wait for condition signal\n");
    rval = pthread_cond_wait(&cond, &mutex);
    if (rval != 0) {
        error(&logger, "thread[%d]: pthread_cond_wait return %d\n", tid, rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 检查  thread_cond_wait 是否会阻塞
    if (signaled == 0) {
        error(&logger, "thread[%d]: pthread_cond_wait did not block at all\n",
              tid);
        cleanup();
        exit(TC_FAIL);
    }
    info(&logger, "thread[%d]: block until signal as expected\n", tid);

    // 检查被 pthread_cond_signal 唤醒后，pthread_cond_wait 能否正确重新获取
    // mutex
    info(&logger, "thread[%d]: trylock mutex (should fail)\n");
    rval = pthread_mutex_trylock(&mutex);
    if (rval != -1) {
        error(&logger,
              "thread[%d]: should not be able to lock the mutex again\n", tid);
        warning(&logger, "FAILED\n");
        cleanup();
        exit(TC_FAIL);
    }
    info(&logger, "thread[%d]: failed to trylock mutex as expected\n", tid);

    info(&logger, "thread[%d]: unlock mutex\n");
    rval = pthread_mutex_unlock(&mutex);
    if (rval != 0) {
        error(&logger, "thread[%d]: failed to release mutex, return %d\n", tid,
              rval);
        cleanup();
        exit(TC_FAIL);
    }

    pthread_exit(NULL);
}

void run() {
    int rval;
    pthread_t thid;

    // 创建新线程
    info(&logger, "creating thread\n");
    rval = pthread_create(&thid, NULL, thread_func, NULL);
    if (rval != 0) {
        error(&logger, "failed to create thread, return %d\n", rval);
        cleanup();
        exit(TC_UNRESOLVED);
    }

    // 等待线程开始
    info(&logger, "main thread: waiting for thread[%d]\n", thid);
    while (!started) { sleep(10); }
    sleep(10);

    // 此时新线程已执行 pthread_cond_wait，mutex 应被释放
    // 测试此时主线程能否获取 mutex
    info(&logger, "main thread: trylock mutex\n");
    rval = pthread_mutex_trylock(&mutex);
    if (rval != 0) {
        error(&logger,
              "main thread: pthread_cond_wait did not release mutex! "
              "pthread_mutex_trylock failed, return %d\n",
              rval);
        cleanup();
        exit(TC_FAIL);
    }
    info(&logger, "main thread: trylock mutex successful as expected\n");

    info(&logger, "main thread: unlock mutex\n");
    rval = pthread_mutex_unlock(&mutex);
    if (rval != 0) {
        error(&logger, "main thread: pthread_mutex_unlock failed, return %d\n",
              rval);
        cleanup();
        exit(TC_FAIL);
    }
    sleep(10);

    // 唤醒新线程
    info(&logger, "main thread: signal to wake up thread[%d]\n", thid);
    signaled = 1;
    rval = pthread_cond_signal(&cond);
    if (rval != 0) {
        error(&logger, "main thread: failed to signal condition, return %d\n",
              rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 等待所有线程结束
    info(&logger, "main thread: joining\n");
    rval = pthread_join(thid, NULL);
    if (rval != 0) {
        warning(&logger, "main thread: failed to join thread, return %d\n",
                rval);
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
