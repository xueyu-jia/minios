/*
 * 测试以下情况：
 *
 * test_1: 传入一个无效的 fd，应返回 -1
 * test_2: read 读取目录时，应返回 -1
 * test_3: 传入一个无效的地址时，应返回 -1
 */

#include <usertest.h>

const char *test_name = "read02";
const char *syscall_name = "read";

logging logger;

const char *dirname = ".";
const char *filename = "read02.txt";
const int BUF_SIZE = 100;

int bad_fd;
int fd2;
int fd3;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    bad_fd = -1;
    fd2 = SAFE_OPEN(dirname, O_RDWR);
    // for test_3
    char write_buf[BUF_SIZE];
    memset(write_buf, '0', BUF_SIZE);
    fd3 = SAFE_OPEN(filename, O_CREAT | O_RDWR);
    SAFE_WRITE(fd3, write_buf, BUF_SIZE);
    SAFE_CLOSE(fd3);
    fd3 = SAFE_OPEN(filename, O_RDWR);
}

void cleanup() {
    SAFE_CLOSE(fd2);
    SAFE_CLOSE(fd3);
    unlink(filename);
    logger_close(&logger);
}

void test_1() {
    char read_buf[BUF_SIZE];
    int n = read(bad_fd, read_buf, BUF_SIZE);
    info(&logger, "test_1: read invalid fd return %d\n", n);
    if (n >= 0) {
        cleanup();
        exit(TC_FAIL);
    }
}

void test_2() {
    char read_buf[BUF_SIZE];
    int n = read(fd2, read_buf, BUF_SIZE);
    info(&logger, "test_2: read a directory return %d\n", n);
    if (n >= 0) {
        cleanup();
        exit(TC_FAIL);
    }
}

void test_3() {
    int n = read(fd3, (void *)-1, BUF_SIZE);
    info(&logger, "test_3: read invalid buf return %d\n", n);
    if (n >= 0) {
        cleanup();
        exit(TC_FAIL);
    }
}

void run() {
    test_1();
    test_2();
    test_3();
    info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
