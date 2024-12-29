/*
 * getcwd 应正确完成功能：
 *  1. buf 和 size 有效
 *  2. buf 为 NULL 且 size 为 0
 *  3. buf 为 NULL 且 size 大于 strlen(path)
 *
 */

#include <usertest.h>

const char *test_name = "getcwd03";
const char *syscall_name = "getcwd";

logging logger;

#define BUF_SIZE 200 // BUF_SIZE must >= MAX_PATH (ie. 128)

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

// buf 和 size 有效
void test_1() {
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    char *p = getcwd(buf, BUF_SIZE);
    if (p == NULL) {
        error(&logger, "test_1: getcwd return NULL, expected a valid pointer\n");
        cleanup();
        exit(TC_FAIL);
    }
    info(&logger, "test_1: getcwd return value: %s\n", p);
    info(&logger, "test_1: getcwd buf value: %s\n", buf);
    if (strcmp(p, buf) != 0 || strcmp(p, "/") != 0) {
        error(&logger, "test_1: failed\n");
        cleanup();
        exit(TC_FAIL);
    }
    info(&logger, "test_1: passed\n");
}

// buf 为 NULL 且 size 为 0
void test_2() {
    char *p = getcwd(NULL, 0);
    if (p == NULL) {
        error(&logger, "test_2: getcwd return NULL\n");
        cleanup();
        exit(TC_FAIL);
    }
    info(&logger, "test_2: getcwd return value: %s\n", p);
    if (strcmp(p, "/") != 0) {
        error(&logger, "test_2: failed\n");
        cleanup();
        exit(TC_FAIL);
    }
    info(&logger, "test_2: passed\n");
}

// buf 为 NULL 且 size 大于 strlen(path)
void test_3() {
    char *p = getcwd(NULL, BUF_SIZE);
    if (p != NULL) {
        error(&logger, "test_3: getcwd return %d, expected NULL\n", p);
        cleanup();
        exit(TC_FAIL);
    }
    if (strcmp(p, "/") != 0) {
        error(&logger, "test_3: failed\n");
        cleanup();
        exit(TC_FAIL);
    }
    info(&logger, "test_3: passed\n");
}

void run() {
    test_1();
    test_2();
    test_3();
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
