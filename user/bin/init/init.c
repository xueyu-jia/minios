#include <malloc.h>
#include <stdio.h>
#include <syscall.h>
#include <time.h>
#include <stdbool.h>
#include <stddef.h>

static void greet() {
    struct tm time;
    get_time(&time);

    printf("\nMemory   %.2fMB\n", total_mem_size() * 1. / 1024 / 1024);
    printf("Datetime %d-%02d-%02d %02d:%02d:%02d\n", time.tm_year + 1900, time.tm_mon + 1,
           time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec);
    printf("\nWelcome to NWPU miniOS!\n");
}

int main(int arg, char *argv[]) {
    int stdin = open("/dev/tty0", O_RDWR);
    int stdout = open("/dev/tty0", O_RDWR);
    int stderr = open("/dev/tty0", O_RDWR);

    greet();

    if (0 != fork()) {
        while (true) { wait(NULL); }
    } else {
        char *arg[2] = {"/bin/shell", NULL};
        char *env[2] = {"PATH=/bin;.", NULL};
        execve(arg[0], arg, env);
    }

    return 0;
}
