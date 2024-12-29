/*
 * 不带 O_CREAT 打开一个不存在的文件，返回 -1
 *
 */
#include <usertest.h>

const char *test_name = "open01";
const char *syscall_name = "open";

logging logger;

const char *filename = "abcdef";
int fd;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
    info(&logger, "starting......\n");
}

void run() {
    fd = open(filename, O_RDWR);

    info(&logger, "open(%s) return %d, expected %d\n", filename, fd, -1);
    if (fd >= 0) {
        warning(&logger, "test FAILED\n");
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "test PASSED\n");
}

void cleanup() {
    if (fd >= 0) {
        SAFE_CLOSE(fd);
        unlink(filename);
    }

    logger_close(&logger);
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
