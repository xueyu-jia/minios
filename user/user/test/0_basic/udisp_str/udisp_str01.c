/*
 * how to test? udisp_str doesn't return anything.
 * how to determine the string pass to udisp_int show in terminal correctly?
 *
 */
#include "usertest.h"

const char *test_name = "udisp_str01";
const char *syscall_name = "udisp_str";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  // udisp_str 不支持部分转义字符如 \t
  // udisp_str("test udisp_str\ttest tab");
  udisp_str(
      "\ntest udisp_str llll\tl\tlllllllll\n\ttest tablllllllllllllllllll\n");

  int len = 256;
  char s[len];
  memset(s, 0, len);
  for (int i = 0; i < 26; i++) {
    s[i] = 'a' + i;
  }

  udisp_str(s);

  // info(&logger, "udisp_str\n");
}

int main(int argc, char *argv[]) {
  setup();
  run();
  cleanup();
  exit(0);
}