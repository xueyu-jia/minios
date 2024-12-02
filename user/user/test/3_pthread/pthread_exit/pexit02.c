/*
 *  测试 pthread_exit 执行后，是否正确退出线程
 *
 *  1. 创建新线程。执行 pthread_exit
 *  2. 主线程通过调用 pthread_join 等线程结束
 *  3. 主线程检查新线程在 pthread_exit 后是否还在运行
 *
 */

#include <usertest.h>

const char *test_name = "pthread_exit01";
const char *syscall_name = "pthread_exit";

logging logger;

static int sem = 0;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void *thread_func(void *arg) {
  pthread_exit(NULL);
  sem = 1;
}

void run() {
  pthread_t thread_id;

  // 创建新线程
  int rval = pthread_create(&thread_id, NULL, thread_func, NULL);
  if (rval != 0) {
    error(&logger, "pthread_create return %d, thread_id: %d\n", rval,
          thread_id);
    cleanup();
    exit(TC_UNRESOLVED);
  }

  // 等待线程结束
  rval = pthread_join(thread_id, NULL);
  if (rval != 0) {
    error(&logger, "pthread_join return %d, expected 0\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  sleep(100);

  if (sem == 1) {
    error(&logger, "test failed. sem = %d, expected %d\n", sem, 0);
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
