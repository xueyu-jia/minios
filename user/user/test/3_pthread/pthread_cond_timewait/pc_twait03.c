/*
 * 测试 pthread_cond_timewait 超时后能否返回 -1 并重新获取 mutex
 *
 * 1. 获取 mutex，调用 pthread_cond_timewait，设置超时时间为 500
 * 2. 查看 pthread_cond_timewait 是否返回 -1
 * 3. 检查超时时间是否正确
 * 4. 检查线程此时是否还持有 mutex
 *
 */
#include <usertest.h>

const char *test_name = "pthread_cond_timewait03";
const char *syscall_name = "pthread_cond_timewait";

logging logger;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
const int timeout = 500;

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);
  info(&logger, "starting\n");
}

void cleanup() {
  int rval;

  rval = pthread_mutex_destroy(&mutex);
  if (rval != 0) {
    error(&logger, "failed to destroy mutex! return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  rval = pthread_cond_destroy(&cond);
  if (rval != 0) {
    error(&logger, "failed to destroy cond! return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }

  logger_close(&logger);
}

void run() {
  int rval;

  int start_time;
  int end_time;

  info(&logger, "acquire mutex\n");
  rval = pthread_mutex_lock(&mutex);
  if (rval != 0) {
    error(&logger, "failed to acquire mutex, return %d\n", rval);
    cleanup();
    exit(TC_UNRESOLVED);
  }

  info(&logger, "timewait for condition signal\n");
  start_time = get_ticks();
  rval = pthread_cond_timewait(&cond, &mutex, &timeout);
  end_time = get_ticks();

  // 检查是否返回 -1
  if (rval != -1) {
    error(&logger, "pthread_cond_timewait return %d\n", rval);
    cleanup();
    exit(TC_FAIL);
  }
  info(&logger, "pthread_cond_timewait return %d as expected\n", rval);

  // 检查阻塞时间
  if (end_time - start_time < timeout) {
    error(&logger, "pthread_cond_timewait block time: %d, expected >= %d\n",
          end_time - start_time, timeout);
    cleanup();
    exit(TC_FAIL);
  }

  // 检查 mutex 的状态
  info(&logger, "main thread: trylock mutex\n");
  rval = pthread_mutex_trylock(&mutex);
  if (rval != -1) {
    error(&logger,
          "pthread_cond_timewait did not hold mutex! "
          "pthread_mutex_trylock failed, return %d\n",
          rval);
    cleanup();
    exit(TC_FAIL);
  }
  info(&logger, "main thread: failed to trylock mutex as expected\n");

  info(&logger, "main thread: unlock mutex\n");
  rval = pthread_mutex_unlock(&mutex);
  if (rval != 0) {
    warning(&logger, "main thread: pthread_mutex_unlock failed, return %d\n",
            rval);
    cleanup();
    exit(TC_UNRESOLVED);
  }

  info(&logger, "PASSED\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
