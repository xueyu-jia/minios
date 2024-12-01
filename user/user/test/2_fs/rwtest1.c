#include "../include/stdio.h"
#include "../include/syscall.h"
#define BUF_SIZE 1024
char read_buf[BUF_SIZE];
int main(int arg, char *argv[]) {
  char filename[] = "rwtest1.txt";
  int fd;
  fd = open(filename, O_CREAT | O_RDWR);
  if (fd == -1) {
    printf("open1 error!");
    exit(-1);
  }
  char w_buf[] = "abc ";
  int ret = 1;
  for (int i = 0; i < 100; i++) {
    ret &= write(fd, w_buf, 3);
  }
  close(fd);

  fd = open(filename, O_RDWR);
  read(fd, read_buf, BUF_SIZE - 1);
  read_buf[BUF_SIZE - 1] = '\0';
  close(fd);
  u32 len = strlen(read_buf);
  printf("bufr len: %d\n", len);
  printf("bufr is: %s\n", read_buf);
  printf("w_buf is:%s", w_buf);
  exit(0);
  return 0;
}