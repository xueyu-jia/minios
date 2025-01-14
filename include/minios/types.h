#pragma once

#include <klib/stdint.h>
#include <klib/sys/types.h>

typedef void (*int_handler)();
typedef void (*fn_task_t)();
typedef void (*irq_handler_t)(int irq);
typedef u32 (*syscall_entry_t)();
