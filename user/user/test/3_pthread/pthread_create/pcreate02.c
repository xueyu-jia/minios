/*
 *  测试 pthread_create 创建线程，检查主线程的 tid 和新线程的 tid
 *  是否不同。检查主线程和新线程的 pid 是否一致。
 *
 *  1. 创建新线程
 *  2. 新线程中调用 get_pid() 并将值保存到全局变量 thread_pid 中
 *  3. 主线程中比较主线程的 tid 和通过 pthread_create 拿到的新线程的 tid. 它
 *      们的值应该不相等
 *  4. 主线程中调用 get_pid() 并将其返回的值和 thread_pid 比较，它们的值应该
 *      相等。
 */

#include <usertest.h>

const char *test_name = "pthread_create02";
const char *syscall_name = "pthread_create";

logging logger;

int thread_pid = -1;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

// 线程函数
void *thread_func(void *arg) {
  thread_pid = get_pid();  // 获取线程的 pid
  pthread_exit(NULL);
}

void run() {
  pthread_t thread_tid;
  pthread_t main_tid;
  int main_pid;
  int rval;

  // 创建新线程
  rval = pthread_create(&thread_tid, NULL, thread_func, NULL);
  if (rval != 0) {
    info(&logger, "pthread_create return %d, expected 0\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  // 比较线程 id
  main_tid = pthread_self();
  info(&logger, "main tid: %d, thread tid: %d\n", main_tid, thread_tid);
  if (main_tid == thread_tid) {
    cleanup();
    exit(TC_FAIL);
  }

  // 等待线程结束
  rval = pthread_join(thread_tid, NULL);
  if (rval != 0) {
    info(&logger, "pthread_join return %d, expected 0\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  // 比较 pid
  main_pid = get_pid();
  info(&logger, "main pid: %d, thread pid: %d\n", main_pid, thread_pid);
  if (main_pid != thread_pid) {
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
