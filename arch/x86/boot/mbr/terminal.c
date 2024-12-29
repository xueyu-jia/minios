#include <terminal.h>
#include <type.h>
#include <ctype.h>
#include <fmt.h>
#include <serial.h>

static inline bool do_putch(char ch) {
    const bool accepted = isprint(ch) || ch == '\n' || ch == '\t';
    if (accepted) { serial_write(ch); }
    return accepted;
}

static char *putch_handler(char *buf, void *user, int len) {
    for (int i = 0; i < len; ++i) {
        if (do_putch(buf[i])) { ++*(int *)user; }
    }
    return buf;
}

static int lvprintf(const char *fmt, va_list ap) {
    int cnt = 0;
    char buf[64] = {};
    strfmt_handler_t handler = {
        .callback = putch_handler,
        .user = &cnt,
    };
    vstrfmtcb(&handler, buf, sizeof(buf), fmt, ap);
    return cnt;
}

int lprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rc = lvprintf(fmt, ap);
    va_end(ap);
    return rc;
}
