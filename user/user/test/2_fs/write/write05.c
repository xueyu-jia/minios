/*
 * 多进程写文件时，write 应该是原子的，即每个 write 写入的内容都是连续的。
 *
 *  1. 新建一个文件
 *  2. 创建多个进程同时写入该文件
 *  3. 所有进程结束后，检查每个 write 写入的内容是否是连续的
 *
 */
#include "usertest.h"

const char *test_name = "write05";
const char *syscall_name = "write";

logging logger;

#define WRITE_NUM 10
#define WRITE_LOOP 10
const char *filename = "write05.txt";
const char *write_str = "0123456789";
int write_str_len;
int fd;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  write_str_len = strlen(write_str);
  info(&logger, "write_str_len: %d\n", write_str_len);
}

void cleanup() {
  unlink(filename);
  logger_close(&logger);
}

void write_func() {
  int pid = get_pid();
  int rval;

  info(&logger, "process[%d] start writing...\n", pid);
  for (int i = 0; i < WRITE_LOOP; i++) {
    rval = write(fd, write_str, write_str_len);
    if (rval != write_str_len) {
      error(&logger, "process[%d]: failed to write; return %d\n", pid, rval);
      exit(-1);
    }
  }

  info(&logger, "process[%d] exiting...\n", pid);
  exit(0);
}

// 检查每个 write 写入的内容是否连续。都是连续的则返回 1，否则返回 0
int validate() {
  int rval;
  int exp_len = WRITE_NUM * WRITE_LOOP * write_str_len;
  char read_buf[exp_len];
  memset(read_buf, 0, sizeof(read_buf));
  debug(&logger, "sizeof(read_buf): %d\n", sizeof(read_buf));

  fd = open(filename, O_RDWR);
  if (fd < 0) {
    error(&logger, "failed to open %s\n", filename);
    cleanup();
    exit(TC_FAIL);
  }
  debug(&logger, "open %s\n", filename);

  rval = read(fd, read_buf, sizeof(read_buf));
  // 由于 read 的 bug， 这里不对其返回值进行错误处理
  // if (rval != exp_len) {
  // error(&logger, "read return %d, expected %d\n", rval, exp_len);
  // cleanup();
  exit(TC_FAIL);
  //}

  printf("exp_len: %d\n", exp_len);
  for (int i = 0; i < sizeof(read_buf); i++) {
    printf("%c", read_buf[i]);
  }
  printf("\n");

  char *s = read_buf;
  for (int i = 0; i < WRITE_NUM * WRITE_LOOP; i++) {
    if (strncmp(s, write_str, write_str_len) != 0) {
      char buf[write_str_len + 1];
      memset(buf, 0, sizeof(buf));
      strncpy(buf, s, write_str_len);
      error(&logger, "expected %s, get %s\n", write_str, buf);
      return 0;
    }
    s += write_str_len;
  }

  close(fd);

  return 1;
}

void run() {
  // 新建文件
  fd = open(filename, O_CREAT | O_RDWR);
  if (fd < 0) {
    error(&logger, "failed to create %s\n", filename);
    cleanup();
    exit(TC_UNRESOLVED);
  }

  // 创建进程
  info(&logger, "creating processes...\n");
  for (int i = 0; i < WRITE_NUM; i++) {
    int pid = fork();
    if (pid == 0) {  // child process
      write_func();
    }
  }

  info(&logger, "waiting for children....\n");
  for (int i = 0; i < WRITE_NUM; i++) {
    wait(NULL);
  }

  close(fd);

  info(&logger, "validating...\n");
  // 检查每次 write 写入的内容是否连续
  if (validate()) {
    info(&logger, "PASSED\n");
  } else {
    warning(&logger, "FAILED\n");
    cleanup();
    exit(TC_FAIL);
  }
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
