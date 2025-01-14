#pragma once

#include <minios/layout.h>

// 减2是因为Arg开头用来保存argv,末尾需要保存一个NULL
#define MAXARG ((ArgLinLimitMAX - ArgLinBase) / 4 - 2)

int kern_execve(const char* path, char* const* argv, char* const* envp);
