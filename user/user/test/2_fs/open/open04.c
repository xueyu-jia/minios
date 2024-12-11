/*
 * 带 O_CREAT 打开一个已经存在的文件，应正确打开文件
 *
 */
#include <usertest.h>

const char *test_name = "open04";
const char *syscall_name = "open";

logging logger;

const char *filename = "open04.txt";
int fd = -1;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    info(&logger, "create file %s\n", filename);
    fd = SAFE_OPEN(filename, O_RDWR | O_CREAT);
    SAFE_CLOSE(fd);
    info(&logger, "setup done\n");
}

void cleanup() {
    if (fd >= 0) {
        SAFE_CLOSE(fd);
        unlink(filename);
    }
    logger_close(&logger);
}

void run() {
    info(&logger, "open existed file with O_CREAT flag\n");
    fd = open(filename, O_RDWR | O_CREAT, I_RW);

    info("open return %d, expected >= 0\n", fd);
    if (fd < 0) {
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
