#include <usertest.h>

const char *test_name = "execve01";

const char *name = "child01";

int main(int argc, char *argv[]) {
  printf("%s: %s executed\n", test_name, name);
  printf("%s: passed\n", test_name);
  exit(TC_PASS);
}
