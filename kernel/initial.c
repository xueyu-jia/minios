#include <minios/syscall.h>
#include <minios/asm.h>
#include <stdbool.h>

void initial() {
    syscall(NR_execve, "/bin/init", NULL, NULL);
    while (true) { halt(); }
}
