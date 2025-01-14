#pragma once

#include <sys/types.h>
#include <io.h>
#include <compiler.h>

int execl(const char *path, const char *arg0, ...);
int execv(const char *path, char *const argv[]);
int execle(const char *path, const char *arg0, ...);
int execve(const char *path, char *const argv[], char *const envp[]);

NORETURN void _exit(int status);

pid_t getpid();
pid_t fork();

int nice(int incr);
