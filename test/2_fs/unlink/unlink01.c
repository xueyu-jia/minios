/*
 * 测试 unlink 能否正确删除文件
 *
 * how to determine weather a file exists since we don't have stat or
 * fstat syscall? use open?
 */

#include <usertest.h>

const char *test_name = "unlink01";
const char *syscall_name = "unlink";

logging logger;

const char *filename = "unlink.txt";

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    int fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
    SAFE_CLOSE(fd);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    int ret = unlink(filename);
    if (ret == -1) {
        info(&logger, "failed to unlink file %s.\n", filename);
        cleanup();
        exit(-1);
    }

    int fd = open(filename, O_RDWR);
    info(&logger, "expected an invalid fd, fd: %d\n", fd);
    if (fd >= 0) {
        close(fd);
        unlink(filename); // try to unlink again
        cleanup();
        exit(-1);
    }

    info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
