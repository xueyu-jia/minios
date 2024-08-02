/*
 * 测试父、子进程是否共享文件描述符，即在读写操作后文件描述符是否会做相应更新。
 *
 *  1. 父进程创建一个空的测试文件并 fork 产生子进程
 *  2. 父进程向文件写入 "abcd"
 *  2. 子进程向文件中写入 "efgh"
 *  3. 父进程等待子进程结束
 *  2. 父进程再向文件写入 "ijkl"
 *  5. 检查此时文件中的内容是否为 "abcdefghijkl"，是则测试通过
 *
 */

#include "usertest.h"

const char *test_name = "fork03";
const char *syscall_name = "fork";

logging logger;

const int BUF_SIZE = 100;
const char *filename = "fork03.txt";
const char *child_write_str = "efgh";
const char *parent_write_str1 = "abcd";
const char *parent_write_str2 = "ijkl";
const char *exp_str = "abcdefghijkl";
int fd;

void cleanup() {
  close(fd);
  unlink(filename);
  logger_close(&logger);
}

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  // 创建测试文件
  fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
  if (fd < 0) {
    error(&logger, "failed to create test file\n");
    cleanup();
    exit(TC_UNRESOLVED);
  }
}

// 返回 1 表示检查通过，返回 0 表示不通过
int check() {
  int rval;
  rval = lseek(fd, 0, SEEK_SET);
  if (rval != 0) {
    error(&logger, "lseek error\n");
    return 0;
  }

  char read_buf[BUF_SIZE];
  memset(read_buf, 0, BUF_SIZE);
  int n = read(fd, read_buf, BUF_SIZE);
  info(&logger, "read string: %s, expected: %s\n", read_buf, exp_str);
  if (strcmp(read_buf, exp_str) != 0) {
    error(&logger, "FAILED\n");
    cleanup();
    exit(TC_FAIL);
  }
}

int run() {
  info(&logger, "fork...\n");
  int pid = SAFE_FORK();
  if (pid == 0) { // child process
    sleep(1000);  // 等待父进程先写入
    info(&logger, "child: write %s\n", child_write_str);
    int n = write(fd, child_write_str, strlen(child_write_str));
    if (n == -1) {
      error(&logger, "child: error write\n");
      exit(TC_UNRESOLVED);
    } else if (n != strlen(child_write_str)) {
      error(&logger, "child: partial write\n");
      exit(TC_UNRESOLVED);
    }
    exit(0);
  }

  // 父进程先写入 "abcd"
  info(&logger, "parent write %s\n", parent_write_str1);
  int n = write(fd, parent_write_str1, strlen(parent_write_str1));
  if (n == -1) {
    error(&logger, "parent: error write\n");
    exit(TC_UNRESOLVED);
  } else if (n != strlen(parent_write_str1)) {
    error(&logger, "parent: partial write\n");
    exit(TC_UNRESOLVED);
  }

  // father process
  info(&logger, "waiting for child\n");
  int status;
  wait(&status);
  if (status != 0) {
    error(&logger, "child process status: %d\n", status);
    cleanup();
    exit(status);
  }

  // 父进程写入 "ijkl"
  info(&logger, "parent write %s\n", parent_write_str2);
  n = write(fd, parent_write_str2, strlen(parent_write_str2));
  if (n == -1) {
    error(&logger, "parent: error write\n");
    exit(TC_UNRESOLVED);
  } else if (n != strlen(parent_write_str2)) {
    error(&logger, "parent: partial write\n");
    exit(TC_UNRESOLVED);
  }

  // 检查文件内容是否为 "abcdefgh"
  check();

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
