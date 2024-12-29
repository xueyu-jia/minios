#pragma once

#include <stdint.h>

#define HANDLER Handler

typedef struct Sigaction {
    int sig;       // 信号的编号
    void *handler; // 该类型信号对应的handler函数指针
                   // 每种类型的信号都会对应一个handler函数,
                   // 最多只有32个handler函数, mingxuan 2021-2-27
    uint32_t arg;  // 传给handler的参数
} Sigaction;

void Handler(Sigaction sigaction);

int kill(int pid, int sig, ...);
int signal(int sig, void *handler);
