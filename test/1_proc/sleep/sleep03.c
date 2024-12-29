/*!
 * 该测例不是一个正经 case，主要用来验证子进程 sleep 的正确性
 *
 * 测例源自对 signal01 失败的修复过程，调试过程中发现 signal01 失败的关键并非
 * signal 系列 syscall 的问题，而是 fork 出的子进程在 sleep 后彻底睡死过去，
 * 以至于其 kill 片段处于 unreachable 的状态，最终父进程 sleep 超时，未接受到
 * 任何信号导致测试失败
 *
 * 以下测例证明，sleep 之前子进程能够被正常调度，sleep 之后调度器依旧在工作，而
 * 父进程的 sleep 并不会出现子进程 sleep 睡死的情况
 */

#include <usertest.h>

int tm_sec() {
    struct tm tm = {};
    get_time(&tm);
    return tm.tm_sec;
}

void delay(int dur_sec) {
    if (dur_sec <= 0) { return; }
    const int now = tm_sec();
    while (1) {
        const int t = tm_sec();
        if (t >= now + dur_sec) { break; }
        if (t < now && t + 60 >= now + dur_sec) { break; }
    }
}

void deamon() {
    if (fork() != 0) { return; }
    int n = 10;
    while (n--) {
        delay(1);
        printf("heartbeat.");
    }
    exit(0);
}

int main() {
    deamon();
    int pid = fork();
    if (pid == 0) {
        printf("child spawned pid=%d\n", get_pid());
        sleep(1);
        printf("echo from child pid=%d\n", get_pid());
        exit(0);
    }
    sleep(1000);
    printf("echo from fa pid=%d\n", get_pid());
    return 0;
}
