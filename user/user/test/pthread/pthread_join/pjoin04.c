/*
 *  测试 pthread_join 一个不是 joinable 的线程
 *
 *  好像不是很好设置传入的线程属性值。MiniOS 并未提供 pthread_attr_init
 *  函数。也不好直接构建 pthread_attr_t 结构体
 *
 *  先不测
 *
 */

#include "usertest.h"

const char *test_name = "pthread_join04";
const char *syscall_name = "pthead_join";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void *thread_func(void *arg) { pthread_exit(NULL); }

void run() {
  pthread_t thread_id;

  pthread_attr_t attr;

  /// ......?
  /*
    pthread_attr_t true_attr = {
        // DETACHED i.e. not joinable
        .detachstate = PTHREAD_CREATE_DETACHED,
        .schedpolicy = SCHED_FIFO,
        .inheritsched = PTHREAD_INHERIT_SCHED,
        .scope = PTHREAD_SCOPE_PROCESS,
        .guardsize = 0,
        .stackaddr_set = 0,
        .stackaddr = 0, // 内核中设置为 p_parent->task.memmap.stack_child_limit
    +
                        // 0x4000 - num_4B;
        .stacksize = 0x4000,
    };
          */

  // 创建新线程
  int rval = pthread_create(&thread_id, NULL, thread_func, NULL);
  if (rval != 0) {
    info(&logger, "pthread_create return %d, thread_id: %d\n", rval, thread_id);
    cleanup();
    exit(TC_FAIL);
  }

  // 等待线程结束
  rval = pthread_join(thread_id, NULL);
  info(&logger, "pthread_join return %d, expected 0\n", rval);
  if (rval != 0) {
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