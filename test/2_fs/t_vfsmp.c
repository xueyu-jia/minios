#include <fcntl.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

int nproc = 20;
void sharedfd(void) {
    int fd, pid, n, nc, np;
    uint32_t i;
    int ntimes = 1000;
    char buf[10];
    uint32_t count[25];
    printf("sharedfd test\n");

    unlink("sharedfd");
    fd = open("sharedfd", O_CREAT | O_RDWR, I_RW);
    if (fd < 0) {
        printf("fstests: cannot open sharedfd for writing");
        return;
    }
    for (i = 1; i < nproc; i++) {
        pid = fork();
        memset(buf, '0' + i, sizeof(buf));
        if (pid == 0) break;
    }
    if (i == nproc) { memset(buf, '0', sizeof(buf)); }
    for (i = 0; i < ntimes; i++) {
        if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
            printf("fstests: write sharedfd failed\n");
            break;
        }
    }
    if (pid == 0)
        exit(-1);
    else {
        for (i = 1; i < nproc; i++) { wait(NULL); }
    }
    close(fd);
    fd = open("sharedfd", 0);
    if (fd < 0) {
        printf("fstests: cannot open sharedfd for reading\n");
        return;
    }
    memset(count, 0, 32);
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        int id = buf[0] - '0';
        if (id >= nproc || id < 0) {
            printf("sharedfd inval content\n");
            exit(-1);
        }
        for (i = 0; i < sizeof(buf); i++) {
            if (buf[i] != buf[0]) break;
        }
        if (i != sizeof(buf)) {
            printf("sharedfd non atomic write\n");
            exit(-1);
        }
        count[id]++;
    }
    close(fd);
    unlink("sharedfd");
    for (i = 0; i < nproc; i++) {
        if (count[i] != ntimes) break;
    }
    if (i == nproc) {
        printf("sharedfd ok\n");
    } else {
        printf("sharedfd oops %d %d\n", i, count[i]);
        exit(-1);
    }
}

int main(int argc, char **argv) {
    sharedfd();
    return 0;
}
