/*
 * 测试 unlink 在文件名过长时能否正确处理
 *
 * how to determine weather a file exists since we don't have stat or
 * fstat syscall? use open?
 */

#include <usertest.h>

const char *test_name = "unlink04";
const char *syscall_name = "unlink";

logging logger;

char *filename =
    "a_very_long_name_in_minios_which_the_length_of_filename_is_limit_to_12";

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int rval = unlink(filename);
  info(&logger, "unlink return %d, expected %d\n", rval, -1);
  if (rval != -1) {
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
