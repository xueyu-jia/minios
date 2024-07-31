#include "usertest.h"

const char *test_name = "execve02";
const char *name = "child02";

const int expected_argc = 3;
const char *const expected_argv[] = {"child02", "p1", "p2"};

int main(int argc, char *argv[]) {
  printf("%s: %s executed\n", test_name, name);
  if (argc != expected_argc) {
    printf("%s: argc: %d, expected %d\n", test_name, argc, expected_argc);
    exit(TC_FAIL);
  }
  if (argv[0] == NULL) {
    printf("%s: argv[0] == NULL\n", test_name);
    exit(TC_FAIL);
  }
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], expected_argv[i]) != 0) {
      printf("%s: argv[%d]: %s, expected: %s\n", test_name, i, argv[i],
             expected_argv[i]);
      exit(-1);
    }
  }
  printf("%s: passed\n", test_name);
  exit(TC_PASS);
}
