/*
 * 测试 write 在以下情况时是否能会返回 -1
 * 1. fd 无效
 * 2. buf 参数无效      // still how to build a bad address?
 * 3. size 无效
 * // TODO 2
 */

#include <usertest.h>

const char *test_name = "write04";
const char *syscall_name = "write";

logging logger;

const char *filename = "write04.txt";
#define BUF_SIZE 100
char write_buf[BUF_SIZE];
int fd;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);
    memset(write_buf, 'a', BUF_SIZE);
    fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
}

void cleanup() {
    if (fd > 0) {
        SAFE_CLOSE(fd);
        unlink(filename);
    }
    logger_close(&logger);
}

void run() {
    int n;

    info(&logger, "test invalid fd\n");
    n = write(-1, write_buf, BUF_SIZE); // invalid fd
    info(&logger, "write return %d, expected %d\n", n, -1);
    if (n != -1) {
        error(&logger, "FAILED\n");
        cleanup();
        exit(TC_FAIL);
    }

    // invalid size
    info(&logger, "test invalid size\n");
    n = write(fd, write, -1);
    info(&logger, "write return %d, expected %d\n", n, -1);
    if (n != -1) {
        error(&logger, "FAILED\n");
        cleanup();
        exit(TC_FAIL);
    }
    /*
  lseek(fd, 0, SEEK_SET);
  char read_buf[BUF_SIZE];
  memset(read_buf, 0, BUF_SIZE);
  read(fd, read_buf, BUF_SIZE);
  for (int i = 0; i < BUF_SIZE; i++) {
    printf("%c ", read_buf[i]);
  }
    */

    // TODO buf 参数无效
    // info(&logger, "test invalid buf\n");
    info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
