#include "stdio.h"
#include "syscall.h"

int main(int argc, char** argv) {
  if (argc != 4) {
    printf("usage: mount %%dev %%target %%fstype\n");
    return -1;
  }
  return mount(argv[1], argv[2], argv[3], 0, NULL);
}