/*
 * 测试 open 能否正常打开文件并写入数据。
 *
 */
#include <usertest.h>

const char *test_name = "open06";
const char *syscall_name = "open";

logging logger;

const char *filename = "open06.txt";
int fd = -1;

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
    fd = open(filename, O_RDWR);

    if (fd < 0) {
        error(&logger, "open return %d, expected a valid fd\n", fd);
        cleanup();
        exit(TC_FAIL);
    }

    char c = 'c';
    int n = write(fd, &c, 1);
    info(&logger, "write reutrn %d\n", n);
    if (n != 1) {
        error(&logger, "can not write to file\n");
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
