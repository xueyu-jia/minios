#include <stdio.h>
#include <syscall.h>

int main(int argc, char** argv) {
  if (argc != 2) {
    printf("usage: mkdir %%path\n");
    return -1;
  }
  return mkdir(argv[1], I_RWX);
}
