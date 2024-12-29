#include <minios/assert.h>
#include <minios/console.h>
#include <minios/x86-asm.h>
#include <stdarg.h>
#include <stddef.h>
#include <fmt.h>

typedef struct fmtinfo_args {
    const char *slevel;
    const char *file;
    const char *func;
    int line;
} fmtinfo_args_t;

static void fmtinfo(char *buffer, size_t size, fmtinfo_args_t *args, const char *fmt, va_list ap) {
    size_t off = 0;
    off +=
        nstrfmt(buffer, size, "%s:%s:%d: %s: ", args->file, args->func, args->line, args->slevel);
    off += vnstrfmt(buffer + off, size - off, fmt, ap);
    off += nstrfmt(buffer + off, size - off, "\n");

    if (off >= size) {
        while (true) { halt(); }
    }
}

void _abort(const char *file, const char *func, int line, const char *fmt, ...) {
    fmtinfo_args_t args = {
        .slevel = "fatal",
        .file = file,
        .func = func,
        .line = line,
    };
    char buffer[256] = {};

    va_list ap;
    va_start(ap, fmt);
    fmtinfo(buffer, sizeof(buffer), &args, fmt, ap);
    va_end(ap);

    kprintf(buffer);

    disable_int();
    while (true) { halt(); }
}

void _warn(const char *file, const char *func, int line, const char *fmt, ...) {
    fmtinfo_args_t args = {
        .slevel = "warn",
        .file = file,
        .func = func,
        .line = line,
    };
    char buffer[256] = {};

    va_list ap;
    va_start(ap, fmt);
    fmtinfo(buffer, sizeof(buffer), &args, fmt, ap);
    va_end(ap);

    kprintf(buffer);
}
