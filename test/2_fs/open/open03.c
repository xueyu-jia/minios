/*
 *  测试进程最多能够打开的文件数
 *
 * minios 中一个进程最大只能打开 64 个文件（由 NR_FILES in proc.h 限制）。然而
 * minios 的文件系统的 inode 数量限制在了 64 个（由 NR_INODE in fs_const.h
 * 限制）。故暂时先不必测这个。
 *
 *
 *
 */
#include <usertest.h>

const char *test_name = "open03";
const char *syscall_name = "open";

logging logger;

#define NUM 10 // ?

char fname[12];
int fd[NUM];

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    for (int i = 0; i < NUM; i++) { fd[i] = -1; }
}

void run() {
    info(&logger, "start...\n");
    for (int i = 0; i < NUM; i++) {
        sprintf(fname, "open%d.txt", i);
        fd[i] = open(fname, O_RDWR | O_CREAT, I_RW);
        if (fd[i] < 0) {
            error(&logger, "open return %d, expected an valid fd\n", fd[i]);
            cleanup();
            exit(TC_FAIL);
        }
    }
    info(&logger, "PASSED\n");
}

void cleanup() {
    for (int i = 0; i < NUM; i++) {
        if (fd[i] >= 0) {
            SAFE_CLOSE(fd[i]);
            sprintf(fname, "open%d.txt", i);
            unlink(fname);
        }
    }
    logger_close(&logger);
}

int main(int argc, char *argv[]) {
    setup();
    run();
    cleanup();
    exit(TC_PASS);
}
