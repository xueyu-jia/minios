/*
 * 测试 pthread_create 创建多个线程，查看他们的 tid 是否不一致
 *
 * 1. 创建多个线程，pthrea_create 返回的 tid 保存到 th_create_ids 数组中
 * 2. 每个线程调用 pthread_self 并将结果保存到一个全局数组 th_self_ids 中
 * 3. 检查 th_self_ids 和 th_create_ids，其中的每个值应都不相等，且与主线程 id
 * 也不相等
 * 过不了， 会导致系统跑飞
 */
#include "usertest.h"

const char *test_name = "pthread_create04";
const char *syscall_name = "pthread_create";

logging logger;

#define THREAD_NUM 40
int thread_args[THREAD_NUM];
static volatile pthread_t th_self_ids[THREAD_NUM + 1];

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  for (int i = 0; i < THREAD_NUM; i++) {
    thread_args[i] = i;
    th_self_ids[i] = -1;
  }
  th_self_ids[THREAD_NUM] = -1;
}

void cleanup() { logger_close(&logger); }

// return true if each value in ids is unique
int check_unique(const char *s, pthread_t ids[]) {
  for (int i = 0; i < THREAD_NUM + 1; i++) {
    for (int j = i + 1; j < THREAD_NUM + 1; j++) {
      if (ids[i] == ids[j]) {
        info(&logger, "%s error: ids[%d] = %d, ids[%d] = %d\n", s, i, ids[i], j,
             ids[j]);
        return 0; // return false
      }
    }
  }
  return 1; // return true
}

// 线程函数
void *thread_func(void *arg) {
  th_self_ids[*(int *)arg] = pthread_self();
  pthread_exit(NULL);
}

void run() {
  // 创建新线程
  pthread_t th_create_ids[THREAD_NUM + 1];
  for (int i = 0; i < THREAD_NUM; i++) {
    int rval =
        pthread_create(th_create_ids + i, NULL, thread_func, thread_args + i);
    if (rval != 0) {
      info(&logger, "pthread_create return %d, expected 0\n", rval);
      cleanup();
      exit(-1);
    }
  }

  // 等待所有线程结束
  for (int i = 0; i < THREAD_NUM; i++) {
    int rval = pthread_join(th_create_ids[i], NULL);
    if (rval != 0) {
      info(&logger, "pthread_join(%d) return %d, expected 0\n",
           th_create_ids[i], rval);
      cleanup();
      exit(-1);
    }
  }

  // 主线程 id
  th_self_ids[THREAD_NUM] = pthread_self();
  th_create_ids[THREAD_NUM] = pthread_self();

  // 检查线程中通过 pthread_self 获得的 id 值
  // 检查 pthread_create 返回的线程 id 值
  if (!check_unique("th_self_ids", th_self_ids) ||
      !check_unique("th_create_ids", th_create_ids)) {
    info(&logger, "failed\n");
    cleanup();
    exit(-1);
  }

  info(&logger, "passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
