#include <signal.h>
#include <stdarg.h>
#include <syscall.h>
#include <stddef.h>

void Handler(Sigaction sigaction) {
    typedef void (*sighandler_t)(int, int);
    ((sighandler_t)sigaction.handler)(sigaction.sig, sigaction.arg);
    int ebp;
    asm volatile("mov %%ebp, %0" : "=r"(ebp) : :);
    sigreturn(ebp);
}

int kill(int pid, int sig, ...) {
    va_list ap = (va_list)((char*)(&sig) + 4);
    Sigaction sigaction = {
        .sig = sig,             // 信号的编号
        .handler = NULL,        // handler函数指针
        .arg = *((uint32_t*)ap) // 传给 handler 的参数
    };
    return sigsend(pid, &sigaction);
}

int signal(int sig, void* handler) {
    return _signal(sig, handler, HANDLER);
}
