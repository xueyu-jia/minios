/*
 * how to test? udisp_int doesn't return anything.
 * how to determine the number pass to udisp_int show in terminal correctly?
 *
 */
#include "usertest.h"

const char *test_name = "udisp_int01";
const char *syscall_name = "udisp_int";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  udisp_int(10);
  info(&logger, "udisp_int\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}