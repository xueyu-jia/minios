/*
 *
 * by lifr 2024.10.16
一个进程（程序）中创建2-3个线程，在一个线程中使用malloc申请一些空间，
观察其他线程是否能够访问这个空间（使用这个空间中的数据），通过这个空间来进行数据通信。
 *
 *
 */
#include "usertest.h"

const char *test_name = "pthread_memory_sharing";

logging logger;

int num_threads = 3;
int *shared_data = NULL;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() {
  free(shared_data);
  logger_close(&logger);
}

// 线程函数1: 申请内存并写入数据
void *thread_func_alloc(void *arg) {
  info(&logger, "Thread alloc: Allocating memory...\n");
  shared_data = (int *)malloc(sizeof(int));
  if (shared_data == NULL) {
    info(&logger, "Memory allocation failed!\n");
    pthread_exit(NULL);
  }
  *shared_data = 100;  // 写入数据
  info(&logger, "Thread alloc: Memory allocated with value %d\n", *shared_data);
  pthread_exit(NULL);
}

// 线程函数2: 读取共享内存数据
void *thread_func_read(void *arg) {
  while (shared_data == NULL) {
    sleep(10);  // 等待内存分配
  }
  info(&logger, "Thread read: Read shared memory value: %d\n", *shared_data);
  pthread_exit(NULL);
}

void run() {
  pthread_t threads[num_threads];

  // 创建线程1: 分配内存并写入数据
  int rval = pthread_create(&threads[0], NULL, thread_func_alloc, NULL);
  info(&logger, "pthread_create return %d, thread_id: %d\n", rval, threads[0]);
  if (rval != 0) {
    cleanup();
    exit(TC_FAIL);
  }

  // 创建其他线程: 读取共享内存数据
  for (int i = 1; i < num_threads; i++) {
    rval = pthread_create(&threads[i], NULL, thread_func_read, NULL);
    info(&logger, "pthread_create return %d, thread_id: %d\n", rval,
         threads[i]);
    if (rval != 0) {
      cleanup();
      exit(TC_FAIL);
    }
  }

  // 等待所有线程完成
  for (int i = 0; i < num_threads; i++) {
    rval = pthread_join(threads[i], NULL);
    info(&logger, "pthread_join return %d\n", rval);
    if (rval != 0) {
      cleanup();
      exit(TC_FAIL);
    }
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
