// used for performance stat
#ifndef BLAME_H
#define BLAME_H
#include <kernel/clock.h>
#include <kernel/type.h>

struct performance_stat {
    char info[16];
    u32  ticks_usage;
    u32  last_record;
};

#define BLAME_TYPES 4
#define BLAME_OTHER 0
#define BLAME_WRT   1
#define BLAME_SYN   2
#define BLAME_READ  3
extern int current_type;
void       stat_init();
void       stat_step(int type);
#endif
