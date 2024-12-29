/*
 * 给 lseek 传入一个无效的 fd，lseek 应返回 -1
 * 给 lseek 传入一个无效的 whence，lseek 应返回 -1
 *
 */

#include <usertest.h>

const char *test_name = "lseek07";
const char *syscall_name = "lseek";

logging logger;

const char *filename = "lseek07.txt";
const char *write_str = "0123456789";

int invalid_fd = -1;
int fd;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    fd = SAFE_OPEN(filename, O_RDWR | O_CREAT);
    SAFE_WRITE(fd, write_str, strlen(write_str));
}

void cleanup() {
    SAFE_CLOSE(fd);
    unlink(filename);

    logger_close(&logger);
}

void tc_run(const char *tc_name, int fd, int offset, int whence) {
    int off = lseek(fd, offset, whence);
    info(&logger, "lseek(%d, %d, %d) return %d, expected %d\n", tc_name, fd, offset, whence, off,
         -1);
    if (off != -1) {
        cleanup();
        exit(TC_FAIL);
    }

    info(&logger, "passed\n", tc_name);
}

void run() {
    tc_run("tc_01", invalid_fd, 4, SEEK_SET);
    tc_run("tc_02", invalid_fd, 4, SEEK_CUR);
    tc_run("tc_03", invalid_fd, -3, SEEK_END);
    tc_run("tc_04", fd, 4, -1);
    tc_run("tc_05", fd, 4, 5);
    tc_run("tc_06", fd, 4, 100);
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
