/*
 * 测试 pthread_mutex_init 能否正确处理传入的属性
 *
 * 目前 MiniOS 的 pthread_mutexattr_t 只有 name 一个属性。
 *
 * 1. 设置 mta 的 name 属性为 "test_mta_name"
 * 2. pthread_mutex_init 使用 mta 初始化一个 mutex
 * 3. 检查 mutex 的 name 和传入的 mta 的 name 是否一致
 *
 */

#include "usertest.h"

const char *test_name = "pthread_mutex_init04";
const char *syscall_name = "pthread_mutex_init";

logging logger;

const char *mta_name = "test_mta_name";

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }


void run() {
  pthread_mutexattr_t mta = {
      .name = mta_name,
  };
  pthread_mutex_t mutex;
  int rval;

  // 初始化 mutex
  if ((rval = pthread_mutex_init(&mutex, &mta)) != 0) {
    info(&logger, "pthread_mutex_init failed, return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  info(&logger, "mutex.name: %s, expected: %s\n", mutex.name, mta_name);
  if (strcmp(mta_name, mutex.name) != 0) {
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
