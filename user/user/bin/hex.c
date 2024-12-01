#include "stdio.h"

int main(int argc, char** argv) {
#define MAX_HEX 512
#define print_hex(c) (((c) >= ' ' && (c) < 0x7F) ? (c) : '.')
  char buf[MAX_HEX];
  if (argc < 4) {
    printf("usage: hex %%file %%block %%offset [%%len]\n");
    return -1;
  }
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    printf("open error");
    return -1;
  }
  int cnt, len, total = MAX_HEX;
  int line_len = 16;
  if (argc == 5) {
    total = min(MAX_HEX, atoi(argv[4]));
  }
  lseek(fd, atoi(argv[2]) * 4096 + atoi(argv[3]), SEEK_SET);
  cnt = read(fd, buf, total);
  for (int i = 0; i < cnt;) {
    len = min(line_len, cnt - i);
    for (int j = 0; j < len; j++) {
      printf("%02x ", (u8)buf[i + j]);
    }
    for (int j = 0; j < len; j++) {
      printf("%c", print_hex(buf[i + j]));
    }
    i += len;
    printf("\n");
  }
  close(fd);
  return cnt;
}