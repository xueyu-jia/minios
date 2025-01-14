#pragma once

#include <sys/types.h>
#include <stdint.h>

typedef struct Sigaction {
    int sig;
    void* handler;
    uint32_t arg;
} Sigaction;

int sigsend(pid_t pid, Sigaction* action);

int kill(pid_t pid, int sig, ...);
int signal(int sig, void* handler);
