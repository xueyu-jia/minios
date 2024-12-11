/*
 * 测试 free_4k 的参数为 NULL 时，能否正确处理
 *
 * 2. 使用 free_4k 回收地址为 NULL 的内存
 *
 */
#include <usertest.h>

const char *test_name = "malloc02";
const char *syscall_name = "malloc";

logging logger;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    int ret = free_4k(NULL);
    info(&logger, "free_4k return %d\n", ret);

    info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
