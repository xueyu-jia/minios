#pragma once

#include <uapi/minios/time.h>
#include <klib/stdint.h>

#define UTC 0
#define LOCAL_TIMEZONE +8 // timestamp use UTC， localtime UTC+8
#define RTC_TIMEZONE UTC  // read cmos rtc as UTC

extern u32 current_timestamp;

u32 mktime(struct tm* time);
struct tm* gmtime(u32 timestamp, struct tm* tm_time);
struct tm* localtime(u32 timestamp, struct tm* tm_time);
u32 get_init_rtc_timestamp();

int kern_get_time(struct tm* time);
