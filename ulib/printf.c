#include <stdio.h>

int tty;

int printf(const char *fmt, ...) {
    int i;
    char buf[PRINT_BUF_LEN];
    va_list arg = (va_list)((char *)(&fmt) + 4);

    i = vsprintf(buf, fmt, arg);

    int n = write(STD_OUT, buf, i);
    if (n == i) {
        return i;
    } else {
        return -1;
    }
}

int fprintf(int fd, const char *fmt, ...) {
    int i;
    char buf[256];
    va_list arg = (va_list)((char *)(&fmt) + 4);

    i = vsprintf(buf, fmt, arg);

    int n = write(fd, buf, i);
    if (n == i) {
        return i;
    } else {
        return -1;
    }
}
