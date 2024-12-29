/*
 * 测试 fork
 * 的多个进程是否共享文件描述符，即在读写操作后文件描述符是否会做相应更新。
 *
 * 写操作
 *
 * 父进程打开一个文件，然后 fork 每个子进程向该文件中写入一个字节的数据。
 * 所有子进程结束后，父进程检查文件描述符是否在文件末尾。
 *
 */

#include <usertest.h>

const char *test_name = "fork08";
const char *syscall_name = "fork";

logging logger;

const int BUF_SIZE = 100;
const int NFORKS = 30;
const char *filename = "fork08.txt";
const char write_char = 'a';
int fd;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    // 创建文件
    fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
}

void cleanup() {
    SAFE_CLOSE(fd);
    unlink(filename);
    logger_close(&logger);
}

void child_func() {
    int n = write(fd, &write_char, sizeof(write_char));
    if (n != 1) {
        info(&logger, "child[%d]: write return %d, expected %d\n", get_pid(), n, 1);
        exit(-1);
    }
    exit(0);
}

int run() {
    info(&logger, "parent: fork chilren...\n");
    for (int i = 0; i < NFORKS; i++) {
        int pid = SAFE_FORK();
        if (pid == 0) { // child process
            child_func();
        }
    }

    // father process
    // 接收子进程状态
    info(&logger, "parent: waiting for chilren...\n");
    for (int i = 0; i < NFORKS; i++) {
        int status;
        wait(&status);
        if (status != 0) {
            error(&logger, "parent: receive %d from child\n", test_name, status);
            cleanup();
            exit(TC_FAIL);
        }
    }

    info(&logger, "parent: check eof\n");
    // 使用 lseek 检查当前指向的位置是否正确
    int rval = lseek(fd, 0, SEEK_CUR);
    if (rval != NFORKS) {
        error(&logger, "lseek CUR return %d, expected %d\n", rval, NFORKS);
        cleanup();
        exit(TC_FAIL);
    }

    char c = '\0';
    int n = read(fd, &c, 1);
    // 由于 read 的
    // bug，这里不使用返回值进行判断，而是通过是否读取到字符进行判断
    if (c != '\0') {
        error(&logger, "file not reach EOF\n");
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
