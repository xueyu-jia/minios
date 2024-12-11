// #include <stdio.h>
// #include <signal/signal.h>
#include <signal.h>
#include <stdio.h>

void handler(int sig, u32 arg) {
    printf("hanlder %d %d", sig, arg);
    for (int i = 0; i < 100; i++) { ; }
}

void main(int arg, char *argv[]) {
    int pid = get_pid();
    int sig = 0;
    int fork_res = fork();
    if (fork_res < 0) { printf("fork error\n"); }
    if (fork_res == 0) {
        int tmp = get_pid();
        int tick = 0;
        // while(1) {
        for (int i = 0; i < 500; i++);
        if (kill(pid, sig, tick++) < 0) { printf("not register"); }
        // }
    } else {
        for (int i = 0; i < 1000; i++);
        signal(sig, handler);

        // while(1) {
        for (int i = 0; i < 1000; i++);
        printf(". ");
        // }
    }
}
