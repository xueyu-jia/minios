#pragma once

#include_next <stdlib.h> // IWYU pragma: export
#include <sys/types.h>
#include <compiler.h>
#include <unistd.h> // IWYU pragma: export
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t pid;
    int nice;
    double vruntime;
    uint64_t sum_cpu_use;
} proc_msg_t;

NORETURN void exit(int status);

pid_t getpid_by_name(const char* name);

void yield();
void sleep(int ms);

void set_rt(bool is_realtime);
void rt_prio(int prio);
void get_proc_msg(proc_msg_t* msg);

char* getenv(const char* key);
