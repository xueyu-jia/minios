/*
 *  不带任何 flag 使用 open 打开一个文件，应能成功打开文件，但不能写入该文件。
 *
 *  1. 预先创建一个测试文件
 *  2. 不带任何 flag 使用 open 打开该文件
 *  3. 检查返回的 fd 是否大于等于 0
 *  4. 检查 write 是否能写入该文件，如能，则失败
 *
 */
#include <usertest.h>

const char *test_name = "open07";
const char *syscall_name = "open";

logging logger;

const char *filename = "open07.txt";
int fd = -1;

int no_flag = 0;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    fd = SAFE_OPEN(filename, O_RDWR | O_CREAT);
    SAFE_CLOSE(fd);
}

void cleanup() {
    if (fd >= 0) {
        SAFE_CLOSE(fd);
        unlink(filename);
    }

    logger_close(&logger);
}

void run() {
    fd = open(filename, no_flag);

    info(&logger, "fd: %d\n", fd);
    if (fd < 0) {
        error("open return %d, expected a valid fd\n", fd);
        cleanup();
        exit(TC_FAIL);
    }

    char c = 'c';
    int n = write(fd, &c, 1);
    info(&logger, "write reutrn %d\n", n);
    if (n > 0) {
        error(&logger, "should not write to file\n");
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
