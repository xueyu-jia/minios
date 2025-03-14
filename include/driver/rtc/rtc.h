#pragma once

#include <uapi/minios/time.h>

#define RTC_REG_ADDR 0x00
#define RTC_REG_DATA 0x01

#define RTC_TIMEZONE UTC

int rtc_probe();
void rtc_get_datetime(struct tm* tm);
