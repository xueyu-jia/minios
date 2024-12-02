/*
 * 测试 fork 是否将原进程中的数据复制了一份。包括栈、堆上的数据。
 *
 * 1. 父进程创建子进程
 * 2. 父进程 sleep(1000)
 * 3. 父进程调用 wait 等待子进程结束
 * 4. 父进程检查运行的时间，应远小于 2000
 *
 */

#include <usertest.h>

const char *test_name = "fork07";
const char *syscall_name = "fork";

logging logger;

struct test_struct {
  char one;
  short two;
  int three;
  void *four;
};

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

int run() {
  int rval;
  int pid;

  info(&logger, "prepare data....\n");
  struct test_struct mystruct = {'1', 2, 3, (void *)4};
  void *malloced = malloc_4k();
  if (malloced == NULL) {
    error(&logger, "malloc_4k error\n");
    cleanup();
    exit(TC_UNRESOLVED);
  }
  info(&logger, "prepare done....\n");
  info(&logger, "fork...\n");
  if ((pid = SAFE_FORK()) == 0) {
    // child process
    if (mystruct.one != '1' || mystruct.two != 2 || mystruct.three != 3 ||
        mystruct.four != (void *)4) {
      error(&logger, "mystruct error\n");
      exit(TC_FAIL);
    }

    // shoud succed
    free_4k(malloced);

    exit(TC_PASS);
  }

  // parent process
  int status;
  if (pid != wait(&status)) {
    error(&logger, "wait return wrong PID\n");
    cleanup();
    exit(TC_FAIL);
  }
  if (status != TC_PASS) {
    error(&logger, "FAILED\n");
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
