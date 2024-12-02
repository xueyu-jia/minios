#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
  char buf[512];
  int fd = open(argv[1], O_WRONLY | O_CREAT, I_R | I_W);
  if (fd < 0) {
    printf("open error");
    return -1;
  }
  lseek(fd, 0, SEEK_END);
  while ((gets(buf) && strlen(buf) != 0)) {
    write(fd, buf, strlen(buf));
  }
  close(fd);
  return 0;
}
