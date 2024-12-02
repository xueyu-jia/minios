/*
 * 测试 malloc_4k 是否能申请多块内存，每次申请是否返回不同的地址
 *
 * 1. 使用 malloc_4k 连续申请多块内存
 * 2. 使用 total_mem_size 来检查系统可用内存是否减少
 * 3. 检查申请返回的地址是否存在重复的现象
 */
#include <usertest.h>

const char *test_name = "malloc02";
const char *syscall_name = "malloc";

logging logger;

#define NUM 40
#define NUM_4K 4096
void *addrs[NUM];

// return true if each value in pids is unique
int check_unique(void *addrs[], int size) {
  for (int i = 0; i < size; i++) {
    for (int j = i + 1; j < size; j++) {
      if (addrs[i] == addrs[j]) {
        error(&logger, "addrs[%d] = %d, addrs[%d] = %d\n", i, addrs[i], j,
              addrs[j]);
        return 0;  // return false
      }
    }
  }
  return 1;  // return true
}

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  memset(addrs, 0, sizeof(addrs));
}

void cleanup() {
  for (int i = 0; i < NUM; i++) {
    free_4k(addrs[i]);
  }

  logger_close(&logger);
}

void run() {
  int mem_before = total_mem_size();
  info(&logger, "mem_before: %d\n", mem_before);
  for (int i = 0; i < NUM; i++) {
    addrs[i] = malloc_4k();
  }
  int mem_after = total_mem_size();
  info(&logger, "mem_after: %d\n", mem_after);

  if (mem_after > mem_before - NUM_4K * NUM) {
    error(&logger, "not enough memory!\n");
    cleanup();
    exit(TC_FAIL);
  }

  if (!check_unique(addrs, NUM)) {
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
