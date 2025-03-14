#pragma once

#include <uapi/minios/time.h>
#include <stdint.h>

struct tm* gmtime(uint32_t timestamp, struct tm* tm_time);
struct tm* localtime(uint32_t timestamp, struct tm* tm_time);
int strftime(char* s, int max, const char* fmt, struct tm* tm_time);

int getticks();
clock_t clock();
int get_time(struct tm* time);
