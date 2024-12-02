/*
 * TODO 搞不清楚它的路径
 */
#include <usertest.h>

const char *test_name = "opendir01";
const char *syscall_name = "opendir";

const char *pathname = ".";
DIR *dirp;
void run() {
  int rval;
  dirp = opendir(pathname);
  printf("opendir return %d\n", dirp);
  rval = closedir(dirp);
  dirp = opendir("/");
  printf("opendir return %d\n", dirp);
  rval = closedir(dirp);
}

int main(int argc, char *argv[]) {
  run();
  exit(0);
}
