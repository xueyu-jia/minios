#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <string.h>

char *getenv(const char *key) {
    static const char **envp = (void *)0xbffff008;
    const char **pp = envp;
    while (*pp != NULL) {
        const char *q = strchr(*pp, '=');
        if (q != NULL && strncmp(key, *pp, q - *pp) == 0) { return (char *)&q[1]; }
        ++pp;
    }
    return NULL;
}

int execve(const char *path, char *const argv[], char *const envp[]) {
    return syscall(NR_execve, path, argv, envp);
}

int execl(const char *path, const char *arg0, ...) {
    const char *argv[MAXARG] = {};

    const char *arg = NULL;
    unsigned int i = 0;

    va_list ap;
    va_start(ap, arg0);
    while ((arg = va_arg(ap, const char *)) != NULL) {
        if (i < MAXARG) { argv[i++] = arg; }
    }
    va_end(ap);

    return execve(path, (char *const *)argv, NULL);
}

int execv(const char *path, char *const argv[]) {
    return execve(path, argv, NULL);
}

int execle(const char *path, const char *arg0, ...) {
    const char *argv[MAXARG] = {};
    const char **envp = NULL;

    const char *arg = NULL;
    unsigned int i = 0;

    va_list ap;
    va_start(ap, arg0);
    while ((arg = va_arg(ap, const char *)) != NULL) {
        if (i < MAXARG) { argv[i++] = arg; }
    }
    envp = va_arg(ap, const char **);
    va_end(ap);

    return execve(path, (char *const *)argv, (char *const *)envp);
}
