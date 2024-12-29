#include <stdio.h>
#include <syscall.h>

int main() {
    int pid = fork();
    int n = 4;
    if (pid) {
        while (n--) {
            printf("I'm fa, son pid = %d", pid);
            // fflush();
            for (int i = 0; i < (int)1e8; i++); // do nothing
        }
    } else {
        while (n--) {
            printf("I'm son");
            // fflush();
            for (int i = 0; i < (int)1e8; i++); // do nothing
        }
    }
}
