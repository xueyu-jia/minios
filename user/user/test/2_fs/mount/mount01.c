
#include <usertest.h>

const char *test_name = "mount01";
const char *syscall_name = "mount";

logging logger;

void setup() { logger_init(&logger, log_filename, test_name, LOG_INFO); }

void cleanup() { logger_close(&logger); }

void run() {
  int ret = mount("/dev/sdb1", "fat0", NULL, NULL, NULL);
  info(&logger, "mount return %d\n", ret);
  info(&logger, "PASSED\n");
}

int main() {
  setup();
  run();
  cleanup();
  exit(TC_PASS);
}
