/*
 *
 * orangefd 未实现 opendir
 * fat32 中 opendir 已弃用
 *
 */
#include "usertest.h"

const char *test_name = "opendir02";
const char *syscall_name = "opendir";

logging logger;
DIR *dirp;
const char *pathname = "fat0/slfjsdfj";

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  DIR *dirp = opendir(pathname);
  printf("opendir return %d\n", dirp);
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}