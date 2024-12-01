#include "stdio.h"
#include "time.h"

int main(int argc, char** argv) {
  struct tm time = {0};
  char buf[32] = {0};
  get_time(&time);
  strftime(buf, 32, "%Y-%m-%d %H:%M:%S", &time);
  printf("%s\n", buf);
  printf("tick:%d\n", get_ticks());
  return 0;
}