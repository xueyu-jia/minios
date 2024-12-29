/*
 * 测试线程从信号处理函数返回时，不会影响正在等待 mutex 的状态
 *
 * 注意：因为 MiniOS 中没有实现 pthread_kill，信号不能发送到某个线程。
 *       这里利用了主线程和新线程的 pid 不相同这个 bug 来使主线程使用 kill
 *       发送信号给子线程
 *
 *
 * 1. 主线程初始化一个全局 mutex，并获取该 mutex
 * 2. 主线程创建一个新线程
 * 3. 新线程中先注册信号的 handler，然后获取该全局 mutex （此处应阻塞）
 * 4. 主线程向新线程发送信号
 * 5. 主线程检查新线程的 handler 是否已处理主线程发送的信号。若已处理，
 *    检查新线程是否还在等待该全局 mutex。
 * 6. 主线程释放 mutex，新线程获取到该 mutex，并运行至结束。若整个程序能
 *    运行结束，则测试通过。
 *
 *
 * result:
 * 当线程阻塞在 pthread_mutex_lock 时，线程在接收到信号时，不会去调用对应的
 * handler 函数
 *
 */

#include <usertest.h>

const char *test_name = "pthread_mutex_lock03";
const char *syscall_name = "pthread_mutex_lock";

logging logger;

const int sig = 10;
int thread_pid; // 新线程 pid
static volatile pthread_mutex_t mutex;
static volatile int handler_run = 0;    // 新线程的 handler 是否已经运行过
static volatile int mutex_accessed = 0; // 新线程是否已获取到 mutex

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

// 新线程信号的 handler
void handler(int sig, uint32_t arg) {
    info(&logger, "thread[%d]: run handler...\n", pthread_self());

    handler_run = 1;
}

void *thread_func(void *arg) {
    thread_pid = get_pid();
    info(&logger, "thread[%d]: start...\n", thread_pid);
    // register signal
    info(&logger, "thread[%d]: register signal...\n", thread_pid);
    int rval = signal(sig, handler);
    if (rval == -1) {
        info(&logger, "thread %d failed to register signal %d\n", pthread_self(), sig);
        pthread_exit(NULL);
    }

    // printf("thread after signal\n");
    //  获取 mutex
    //  sleep(100);
    info(&logger, "thread[%d]: acquire mutex...\n", thread_pid);
    pthread_mutex_lock(&mutex);
    mutex_accessed = 1;
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

void run() {
    int rval;
    pthread_t thread_id;

    // 初始化锁
    info(&logger, "initializing mutex...\n");
    rval = pthread_mutex_init(&mutex, NULL);
    if (rval != 0) {
        info(&logger, "pthread_mutex_init failed, return %d\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 主线程获取 mutex
    info(&logger, "main: acquire mutex\n");
    rval = pthread_mutex_lock(&mutex);
    if (rval != 0) {
        info(&logger, "pthread_mutex_lock return %d, expected 0\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 创建新线程
    info(&logger, "main: creating thread...\n");
    rval = pthread_create(&thread_id, NULL, thread_func, NULL);
    if (rval != 0) {
        info(&logger, "pthread_create return %d, expected 0\n", rval);
        cleanup();
        exit(TC_FAIL);
    }

    // 等待新线程注册信号的 handler
    // TODO 使用信号量或消息队列来进行同步会更优雅
    sleep(50);

    // 向新线程发送信号
    info(&logger, "main: send signal...\n");
    Sigaction sigaction = {.sig = sig, .handler = NULL, .arg = 0};
    sigsend(thread_pid, &sigaction);

    // printf("main after kill\n");
    //  检查新线程的 handler 是否已运行
    info(&logger, "main: check if handler runned...\n");
    int i = 0;
    while (handler_run == 0) {
        sigsend(thread_pid, &sigaction);
        sleep(50);
    }
    sleep(50);

    // 检查新线程是否已获取了 mutex
    info(&logger, "main: check if child acquire mutex...\n");
    if (mutex_accessed != 0) {
        info(&logger, "error: thread acquire mutex before main thread release!\n", test_name);
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "main: release mutex...\n");
    pthread_mutex_unlock(&mutex);
    info(&logger, "main: waiting for child join...\n");
    pthread_join(thread_id, NULL);

    info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
