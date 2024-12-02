#include <stdio.h>
#include <syscall.h>

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("usage: rmdir %%path\n");
    return -1;
  }
  return rmdir(argv[1]);
}
