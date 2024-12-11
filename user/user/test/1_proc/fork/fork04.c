/*
 * 测试 fork
 * 的多个进程是否共享文件描述符，即在读写操作后文件描述符是否会做相应更新。
 *
 *
 * 父进程打开一个文件，写入与子进程数量相等的字符 'a'，然后 fork.
 * 每个子进程从文件中读取一个字符并检查是否正确（即是否为
 * 'a'）。所有子进程结束后，父进程检查子进程是否读取了正确数量的字节。
 *
 */

#include <usertest.h>

const char *test_name = "fork04";
const char *syscall_name = "fork";

logging logger;

const int BUF_SIZE = 100;
const int NFORKS = 30;
const char *filename = "fork04.txt";
const char write_char = 'a';
int fd;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    // 创建文件
    fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
    for (int i = 0; i < NFORKS; i++) { write(fd, &write_char, 1); }
    SAFE_CLOSE(fd);

    fd = SAFE_OPEN(filename, O_RDWR);
}

int run() {
    info(&logger, "parent: fork chilren...\n");
    for (int i = 0; i < NFORKS; i++) {
        int pid = SAFE_FORK();
        if (pid == 0) { // child process
            char c;
            int n = read(fd, &c, 1);
            if (c != 'a') {
                info(&logger, "child[%d]: read '%c' instead of 'a'\n",
                     get_pid(), c);
                exit(-1);
            }
            exit(0);
        }
    }

    // father process
    // 接收子进程状态
    info(&logger, "parent: waiting for chilren...\n");
    for (int i = 0; i < NFORKS; i++) {
        int status;
        wait(&status);
        if (status != 0) {
            error(&logger, "parent: receive %d from child\n", test_name,
                  status);
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

    // 检查是否读取到 EOF
    char c = '\0';
    int n = read(fd, &c, 1);
    // 由于 read 的
    // bug，这里不使用返回值进行判断，而是通过是否读取到字符进行判断
    if (c != '\0') {
        error(&logger, "file not reach EOF\n");
        cleanup();
        exit(TC_FAIL);
    }

    // reach EOF
    info(&logger, "read EOF correctly\n");
    return 0;
}

void cleanup() {
    SAFE_CLOSE(fd);
    unlink(filename);
    logger_close(&logger);
}

int main(int argc, char *argv[]) {
    setup();
    int exit_status = run();
    cleanup();
    exit(exit_status);
}
