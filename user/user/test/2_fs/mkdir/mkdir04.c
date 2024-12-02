/*
 * fat32
 * 测试 mkdir 能否正常创建一个新的目录
 *
 * 使用 chdir 测试是否创建成功
 */

#include <usertest.h>

const char *test_name = "mkdir04";
const char *syscall_name = "mkdir";

logging logger;

// why pathname error?
// const char *dirname = "fat0/dir01";
const char *dirname = "dir01";

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() {
  rmdir(dirname);
  logger_close(&logger);
}

void run() {
  int rval = mkdir(dirname, I_RWX);
  info(&logger, "mkdir %s return %d, expected 0\n", dirname, rval);
  if (rval != 0) {
    cleanup();
    exit(-1);
  }

  rval = chdir(dirname);
  info(&logger, "chdir return %d\n", rval);
  if (rval == 0) {
    cleanup();
    exit(-1);
  }

  info(&logger, "test passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
