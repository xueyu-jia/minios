/*
 * 测试主线程和新线程是否是异步的。
 *
 * 注意：因为 MiniOS 中没有实现 pthread_kill，信号不能发送到某个线程。
 *       这里利用了主线程和新线程的 pid 不相同这个 bug 来使主线程使用 kill
 *       发送信号给子线程
 *
 * 1. 主线程创建新线程，然后主线程 sleep(1000)
 * 2. 新线程获取它的 pid 值保存到全局变量 thread_pid 中
 * 3. 新线程注册信号，然后跑在一个死循环里
 * 4. 主线程 sleep 结束后，给新线程发送信号。新线程信号的 handler 中调用
 *    pthread_exit 退出线程。如果 MiniOS 不是异步的，即主线程不会在
 *    新线程结束前被调度，则主线程永远不会运行到 sigsend。则新线程也永远不
 *    会结束。
 * 5. 如果程序能正常结束，则说明 MiniOS 是异步的
 *
 */

#include <usertest.h>

const char *test_name = "pthread_create03";
const char *syscall_name = "pthread_create";

logging logger;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

// 给新线程发的信号
const int sig = 10;
int thread_pid;

// 信号的 handler
// 线程在此退出
void handler(int sig, uint32_t arg) {
    info(&logger, "sig handler tid: %d\n", pthread_self());
    pthread_exit(NULL);
}

// 线程函数
void *thread_func(void *arg) {
    thread_pid = get_pid();
    // info(&logger, "thread tid: %d\n", pthread_self());
    //  register signal
    int rval = signal(sig, handler);
    if (rval == -1) {
        info(&logger, "thread %d failed to register signal %d\n",
             pthread_self(), sig);
        pthread_exit(NULL);
    }
    // 死循环
    while (1) { sleep(10); }
    // unreachable!
    pthread_exit(NULL);
}

void run() {
    pthread_t thread_id;
    pthread_t main_id;
    int main_pid;
    int rval;

    // 创建新线程
    rval = pthread_create(&thread_id, NULL, thread_func, NULL);
    if (rval != 0) {
        info(&logger, "pthread_create return %d, expected 0\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    // info(&logger, "main tid: %d\n", pthread_self());
    sleep(10);

    Sigaction sigaction = {.sig = sig, .handler = NULL, .arg = 0};
    sigsend(thread_pid, &sigaction);

    // 等待线程结束
    rval = pthread_join(thread_id, NULL);
    if (rval != 0) {
        info(&logger, "pthread_join return %d, expected 0\n", rval);
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
