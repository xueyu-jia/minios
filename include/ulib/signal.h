#pragma once

#include <stdint.h>

typedef struct Sigaction {
    int sig;
    void* handler;
    uint32_t arg;
} Sigaction;

int kill(int pid, int sig, ...);
int signal(int sig, void* handler);
