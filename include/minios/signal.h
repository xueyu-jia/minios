#pragma once

#include <uapi/minios/signal.h>
#include <klib/stdint.h>

typedef struct Sigaction {
    int sig;
    void* handler;
    u32 arg;
} Sigaction;

int kern_signal(int sig, void* handler, void* _Handler);
int kern_sigsend(int pid, Sigaction* action);
void kern_sigreturn(int ebp);
