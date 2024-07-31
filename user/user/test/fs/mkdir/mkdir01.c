/*
 * 测试 mkdir 能否正常创建一个新的目录
 *
 * TODO  how to detemine a directory exists?
 *
 *
 * mkdir 成功时会返回 0
 * 失败时可能会返回：
 * DIR_PATH_INEXISTE    -1
 * DIR_PATH_REPEATED    -2
 * DIR_FILE_OCCUPIYED   -3
 * 注：VFS update: mkdir 失败时返回-1,暂未实现错误码
 */

#include "usertest.h"

const char *test_name = "mkdir01";
const char *syscall_name = "mkdir";

logging logger;

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
  // info(&logger, "done\n");
  //  how to detemine a directory exists?
  //  readdir?
  info(&logger, "mkdir return %d\n", rval);
  /*
  if (chdir(dirname) == 0) {
    info(&logger, "mkdir failed\n");
    cleanup();
    exit(-1);
  }
   */
  info(&logger, "test passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
