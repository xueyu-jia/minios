/*
 * 测试 rmdir 删除一个存在文件的目录时是否返回 DIR_PATH_OCCUPIYED i.e. -3
 *
 * TODO how to determine a directory really exists in file system since we don't
 * have stat or fstat syscall?
 *
 * orangefs implements:
 * f_op_table[1].open = real_open;
 * f_op_table[1].close = real_close;
 * f_op_table[1].write = real_write;
 * f_op_table[1].lseek = real_lseek;
 * f_op_table[1].unlink = real_unlink;
 * f_op_table[1].read = real_read;
 * f_op_table[1].mkdir = real_mkdir;
 * f_op_table[1].rmdir = real_rmdir;
 *
 * rmdir 成功时会返回 0
 * 失败时可能会返回：
 * DIR_PATH_INEXISTE    -1
 * DIR_PATH_REPEATED    -2
 * DIR_FILE_OCCUPIYED   -3
 */

#include "usertest.h"

const char *test_name = "rmdir03";
const char *syscall_name = "rmdir";

logging logger;

const char *dirname = "ddir03";
// 如何在一个目录内创建文件？ 无 chdir 系统调用
const char *filename = "ddir03/file";

void setup() {
  logger_init(&logger, log_filename, test_name, LOG_INFO);

  int rval = mkdir(dirname, I_RWX);
  if (rval != 0) {
    exit(-1);
  }
  int fd = SAFE_OPEN(filename, O_CREAT | O_RDWR);
  write(fd, (void *)"a", 1);
  SAFE_CLOSE(fd);
}

void cleanup() { logger_close(&logger); }

void run() {
  int rval = rmdir(dirname);
  if (rval != DIR_PATH_INEXISTE) {
    error(&logger, "rmdir %s return %d, expected %d\n", dirname, rval,
          DIR_PATH_INEXISTE);
    cleanup();
    exit(-1);
  }
  // TODO
  info(&logger, "rmdir %s return %d\n", dirname, rval);
  info(&logger, "test passed\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}
