/*
 * how to test sigreturn?
 * sigreturn 不应该由用户程序来调用。
 * void sigreturn(int ebp); 中 ebp 的值表示什么呢？
 *
 *
 * linux test project 并未对该系统调用进行测试
 *
 */

#include <usertest.h>

const char *test_name = "sigreturn01";
const char *syscall_name = "sigreturn";

logging logger;

int sig_num = 1;

void handler(int sig, uint32_t arg) {
    printf("%s: handler %d %d\n", test_name, sig, arg);
}

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
