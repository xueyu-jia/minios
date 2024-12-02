/*
 * 测试 pthread_join 是否会阻塞直至线程结束
 *
 * 1. 创建新线程，新线程 sleep(300)。之后新线程设置全局变量 end_exec 为 1
 * 2. 主线程应等待线程结束。如果主线程未等待线程结束，则会读取到 end_exec 为 0
 *
 */

#include <usertest.h>

const char *test_name = "pthread_join01";
const char *syscall_name = "pthead_join";

logging logger;

// 线程是否执行完
static int end_exec = 0;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void *thread_func(void *arg) {
  sleep(300);
  end_exec = 1;
  pthread_exit(NULL);
}

void run() {
  pthread_t thread_id;

  // 创建新线程
  int rval = pthread_create(&thread_id, NULL, thread_func, NULL);
  info(&logger, "pthread_create return %d, thread_id: %d\n", rval, thread_id);
  if (rval != 0) {
    cleanup();
    exit(TC_FAIL);
  }

  // 判断 pthread_join 返回值是否正确
  rval = pthread_join(thread_id, NULL);
  info(&logger, "pthread_join return %d, expected 0\n", rval);
  if (rval != 0) {
    cleanup();
    exit(TC_FAIL);
  }

  // 此时读取到 end_exec 为 0 的话，说明线程未执行完
  if (end_exec == 0) {
    info(&logger, "main did not wait for thread to finish!\n");
    cleanup();
    exit(TC_FAIL);
  }

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
