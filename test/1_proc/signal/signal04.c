/*
 * 线程信号测试
 */
#include <usertest.h>

const char *test_name = "signal04";
const char *syscall_name = "signal";

logging logger;

typedef struct atomic {
    int counter;
} atomic_t;
atomic_t triggered = {.counter = 0};

const int set_sig = 0;
const int set_arg = set_sig;

static inline void atomic_inc(atomic_t *v) {
    asm volatile("lock; incl %0" : "+m"(v->counter));
}

void sighandler(int sig, uint32_t arg) {
    atomic_inc(&triggered);
}

void worker(void *arg) {}

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void run() {
    int rval = signal(set_sig, sighandler);
    if (rval == -1) {
        info(&logger, "failed to register signal %d\n", set_sig);
        cleanup();
        exit(-1);
    }

    kill(get_pid(), set_sig, set_arg);
    pthread_t tid;
    pthread_create(&tid, NULL, worker, NULL);
    yield();

    sleep(100);
    yield();

    if (triggered.counter != 1) {
        info(&logger, "signal processed more than once (%d) among threads!", triggered.counter);
        cleanup();
        exit(-1);
    }

    info(&logger, "passed\n");
}

void cleanup() {
    logger_close(&logger);
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
