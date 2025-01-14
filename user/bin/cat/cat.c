#include <stdio.h>

int main(int argc, char* argv[]) {
#define MAX_CAT 128
    char buf[MAX_CAT];
    if (argc != 2) {
        printf("usage: cat %%path\n");
        return -1;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("open error");
        return -1;
    }
    int cnt;
    do {
        cnt = read(fd, buf, MAX_CAT);
        write(STD_OUT, buf, cnt);
    } while (cnt == MAX_CAT);
    printf("\n");
    close(fd);
    return 0;
}
