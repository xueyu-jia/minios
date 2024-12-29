/*
 * execve 运行一个不存在的文件时应失败
 * execve 执行失败时应返回 -1
 *
 * execve 的返回值应该是一个 int 类型的值。然而 minios 的返回值是 uint32_t 类型
 */

#include <usertest.h>

const char *test_name = "execve04";
const char *syscall_name = "execve";

logging logger;

const char *child_name = "abcd";

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    int ret = execve(child_name, NULL, NULL);
    info(&logger, "execve return %d, exepcted %d\n", ret, -1);
    if (ret != -1) {
        error(&logger, "failed\n");
        cleanup();
        exit(TC_FAIL);
    }
    info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
