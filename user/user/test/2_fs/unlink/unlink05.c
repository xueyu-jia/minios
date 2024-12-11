/*
 * 将 NULL 作为参数传递给 unlink 时，unlink 应返回 -1
 *
 * how to determine weather a file exists since we don't have stat or
 * fstat syscall? use open?
 */

#include <usertest.h>

const char *test_name = "unlink05";
const char *syscall_name = "unlink";

logging logger;

int fd;
const char *filename = "ul05.txt";
const char *write_data = "something";

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
}

void cleanup() {
    logger_close(&logger);
}

void run() {
    /*
    fd = SAFE_OPEN(filename, O_RDWR | O_CREAT);
    SAFE_WRITE(fd, write_data, strlen(write_data));
    SAFE_CLOSE(fd);

    fd = SAFE_OPEN(filename, O_RDWR);
    SAFE_CLOSE(fd);
    fd = SAFE_OPEN(filename, O_RDWR);
    SAFE_CLOSE(fd);
    fd = SAFE_OPEN(filename, O_RDWR);
    SAFE_CLOSE(fd);
    */

    int rval = unlink(NULL);
    info(&logger, "unlink(NULL) return %d, expected %d\n", rval, -1);
    if (rval != -1) {
        cleanup();
        exit(-1);
    }

    /*
    int fd = open(filename, O_RDWR);
    info(&logger, "expected an invalid fd, fd: %d\n", fd);
    if (fd >= 0) {
      info(&logger, "test failed\n");
      close(fd);
      exit(-1);
    }
    */

    info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(0);
}
