/*
 * 打开一个文件，fork 一个子进程。子进程向文件写入数据，然后在未调用
 * close 关闭文件的情况下，使用 exit 结束进程。
 * 父进程此时读取方文件，如果能读取到子进程写入的数据，则通过测试。
 */

#include "usertest.h"

const char *test_name = "exit02";
const char *syscall_name = "exit";

logging logger;

const int BUF_SIZE = 10;
char *filename = "test.txt";
char *wdata = "abcd";
int len = 0;
int fd;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
  len = strlen(wdata);
}

void cleanup() {
  SAFE_CLOSE(fd);
  unlink(filename);
  logger_close(&logger);
}

void run() {
  int pid = SAFE_FORK();
  if (pid == 0) {  // child process
    write(fd, wdata, len);
    exit(0);
  }

  // parent process
  char rdata[BUF_SIZE];
  memset(rdata, 0, BUF_SIZE);
  int status;
  wait(&status);
  lseek(fd, 0, SEEK_SET);
  int n = read(fd, rdata, len);
  rdata[n] = 0;  // add '\0'
  info(&logger, "expected read: %d, actual read: %d\n", len, n);
  info(&logger, "expected data: %s, actual data: %s\n", wdata, (char *)rdata);
  if (n != len || strncmp(rdata, wdata, len) != 0) {
    cleanup();
    exit(-1);
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
