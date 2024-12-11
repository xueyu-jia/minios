/*
 * 测试子进程 close 文件描述符不会影响到父进程。
 *
 * 1. 父进程创建测试文件，并写入一个字符 'a'
 * 2. fork 一个子进程，该子进程 close 文件描述符并退出
 * 3. fork 另一个子进程，该子进程读取一个字符，检查是否为 'a'，并退出
 * 4. 父进程检查文件是否已读到未尾
 *
 */

#include <usertest.h>

const char *test_name = "fork05";
const char *syscall_name = "fork";

logging logger;

const int BUF_SIZE = 100;
const int NFORKS = 30;
const char *filename = "fork05.txt";
const char write_char = 'a';
int fd;

void setup() {
    logger_init(&logger, log_filename, test_name, LOG_INFO);

    // 创建文件，写入一个字符 'a'
    fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
    write(fd, &write_char, 1);
    SAFE_CLOSE(fd);

    fd = SAFE_OPEN(filename, O_RDWR);
}

void fork_child1() {
    info(&logger, "parent: fork chilren1...\n");
    // 创建子进程并关闭 fd
    if (SAFE_FORK() == 0) { // child process
        close(fd);
        exit(0);
    }

    // 接收子进程状态
    info(&logger, "parent: waiting for chilren1...\n");
    int status;
    wait(&status);
    if (status != 0) {
        error(&logger, "parent: receive %d from child\n", test_name, status);
        cleanup();
        exit(TC_FAIL);
    }
}

void fork_child2() {
    info(&logger, "parent: fork chilren2...\n");

    // 创建子进程并读取一个字符
    if (SAFE_FORK() == 0) { // child process
        char c;
        int n = read(fd, &c, 1);
        if (c != 'a') {
            info(&logger, "child[%d]: read '%c' instead of 'a'\n", get_pid(),
                 c);
            exit(-1);
        }
        close(fd);
        exit(0);
    }

    // 接收子进程状态
    info(&logger, "parent: waiting for chilren2...\n");
    int status;
    if (status != 0) {
        error(&logger, "parent: receive %d from child\n", test_name, status);
        cleanup();
        exit(TC_FAIL);
    }
}

int run() {
    fork_child1();
    fork_child2();

    // 检查是否读到 EOF
    info(&logger, "parent: check eof\n");
    char c = '\0';
    int n = read(fd, &c, 1);
    // 由于 read 的
    // bug，这里不使用返回值进行判断，而是通过是是否读取到字符进行判断
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
