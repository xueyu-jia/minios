#include <uapi/minios/syscall.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <stddef.h>

static void signal_handler_wrapper(Sigaction sigaction) {
    typedef void (*sighandler_t)(int, int);
    ((sighandler_t)sigaction.handler)(sigaction.sig, sigaction.arg);
    int ebp = 0;
    asm volatile("mov %%ebp, %0" : "=r"(ebp) : :);
    syscall(NR_sigreturn, ebp);
}

int kill(int pid, int sig, ...) {
    va_list ap;
    va_start(ap, sig);
    uint32_t arg = va_arg(ap, uint32_t);
    va_end(ap);

    Sigaction sigaction = {
        .sig = sig,
        .handler = NULL,
        .arg = arg,
    };
    return syscall(NR_sigsend, pid, &sigaction);
}

int signal(int sig, void* handler) {
    return syscall(NR_signal, sig, handler, signal_handler_wrapper);
}
