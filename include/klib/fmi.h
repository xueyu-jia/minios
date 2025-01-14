#pragma once

#include <klib/stddef.h>
#include <klib/sys/types.h>

typedef struct free_memory_info {
    size_t count;
    struct {
        phyaddr_t base;
        phyaddr_t limit;
    } page_ranges[0];
} free_memory_info_t;

#define __fmi_ptr() ((free_memory_info_t*)u2ptr(0x007ff000))
