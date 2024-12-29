#include <stdio.h>
#include <syscall.h>
#include <stddef.h>

#define assert(x) \
    if (!(x)) { printf("assertion failed: %s at %s:%d\n", #x, __FILE__, __LINE__); }
void test_fork_wait1(void) {
    int pid = fork();

    assert(pid >= 0);

    if (pid == 0) exit(114);

    int wstatus;
    assert(pid == wait(&wstatus));

    assert(wstatus == 114);

    printf("test_fork_wait1 passed!\n");
}

void test_fork_wait2(void) {
    int pid = fork();

    assert(pid >= 0);

    if (pid == 0) exit(514);

    assert(pid == wait(NULL));

    printf("test_fork_wait2 passed!\n");
}

void test_empty_wait(void) {
    assert(wait(NULL) == -1);

    printf("test_empty_wait passed!\n");
}

void test_fork_limit(void) {
    int xor_sum = 0;

    for (int i = 1; i <= 45; i++) {
        int pid = fork();

        assert(pid >= 0);

        if (pid == 0) exit(0);

        xor_sum ^= pid;
    }

    assert(fork() == -1);

    int wait_cnt = 0, wait_pid;
    while ((wait_pid = wait(NULL)) >= 0) wait_cnt++, xor_sum ^= wait_pid;

    assert(wait_cnt == 45);
    assert(xor_sum == 0);

    printf("test_fork_limit passed!\n");
}

void test_wait_is_sleeping(void) {
    int rt_pid = get_pid();

    for (int i = 1; i <= 45; i++) {
        int pid = fork();

        assert(pid >= 0);

        if (pid > 0) {
            int wstatus;
            assert(pid == wait(&wstatus));
            assert(wstatus == 42);
            if (get_pid() != rt_pid) exit(42);
            break;
        }
    }

    if (get_pid() != rt_pid) {
        int la_ticks = get_ticks();

        for (int i = 1; i <= (int)500; i++) {
            int now_ticks = get_ticks();
            assert(now_ticks - la_ticks <= 3);
            la_ticks = now_ticks;
        }

        exit(42);
    }

    printf("test_wait_is_sleeping passed!\n");
}

int main() {
    test_fork_wait1();
    test_fork_wait2();
    test_empty_wait();
    test_fork_limit();
    test_wait_is_sleeping();
    printf("all tests passed!\n");
    return 0;
}
