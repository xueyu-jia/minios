/*
 * 测试 execve 能否正确传递 argv 参数
 * execve 执行成功时应该能读取到正确的 argc 值和 argv 数组
 *
 * refer: child02.c
 */

#include <usertest.h>

const char *test_name = "execve02";
const char *syscall_name = "execve";

logging logger;

const char *child_name = "child02";

const char *const para[] = {"p1", "p2", NULL};

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    // int ret = execve(child_name, para, NULL);
    int ret = execve(child_name, para, NULL);
    // should not return
    error(&logger, "execve return %d\n", ret);
    error(&logger, "failed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_FAIL);
}
