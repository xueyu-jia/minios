#pragma once
#include "const.h"
PUBLIC u32 do_execve(const char* path, char* const* argv, char* const* envp);
PUBLIC u32 kern_execve(const char* path, char* const* argv, char* const* envp);
PUBLIC u32 sys_execvp();
PUBLIC u32 sys_execv();
PUBLIC u32 sys_execve();
