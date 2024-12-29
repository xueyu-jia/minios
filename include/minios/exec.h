#pragma once
#include <minios/const.h>
u32 do_execve(const char* path, char* const* argv, char* const* envp);
u32 kern_execve(const char* path, char* const* argv, char* const* envp);
u32 sys_execvp();
u32 sys_execv();
u32 sys_execve();
