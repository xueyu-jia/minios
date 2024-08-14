#include <kernel/blame.h>
struct performance_stat blame_dash[BLAME_TYPES] = {
	[BLAME_OTHER] = {"other", 0, 0},
	[BLAME_WRT] = {"wrt", 0, 0},
	[BLAME_SYN] = {"syn", 0, 0},
	[BLAME_READ] = {"rd", 0, 0},
};

int current_type;

void stat_init() {
	current_type = BLAME_OTHER;
	blame_dash[BLAME_OTHER].last_record = ticks;
}

void stat_step(int type) {
	int tick = ticks;
	blame_dash[current_type].ticks_usage += tick - blame_dash[current_type].last_record;
	current_type = type;
	blame_dash[type].last_record = tick;
}
