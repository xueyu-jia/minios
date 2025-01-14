#pragma once

#include <klib/stdint.h>

#define BLAME_TYPES 4
#define BLAME_OTHER 0
#define BLAME_WRT 1
#define BLAME_SYN 2
#define BLAME_READ 3

struct performance_stat {
    char info[16];
    u32 ticks_usage;
    u32 last_record;
};

extern int current_type;

void stat_init();
void stat_step(int type);
