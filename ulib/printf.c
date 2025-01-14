#include <stdio.h>
#include <fmt.h>

typedef struct {
    int fd;
    int count;
} vfprint_arg_t;

static char *putch_handler(char *buf, void *user, int len) {
    vfprint_arg_t *arg = (vfprint_arg_t *)user;
    int resp = write(arg->fd, buf, len);
    if (resp > 0) { arg->count += resp; }
    return buf;
}

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const int ret = vfprintf(stdout, fmt, ap);
    va_end(ap);
    return ret;
}

int vfprintf(int fd, const char *fmt, va_list ap) {
    int cnt = 0;
    char buf[64] = {};
    vfprint_arg_t arg = {
        .fd = fd,
        .count = 0,
    };
    strfmt_handler_t handler = {
        .callback = putch_handler,
        .user = &arg,
    };
    vstrfmtcb(&handler, buf, sizeof(buf), fmt, ap);
    return cnt;
}

int fprintf(int fd, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const int ret = vfprintf(fd, fmt, ap);
    va_end(ap);
    return ret;
}
