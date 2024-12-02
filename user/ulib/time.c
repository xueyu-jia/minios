#include <const.h>
#include <stdio.h>
#include <time.h>
#include <type.h>
PRIVATE struct tm* _gmtime(u32 timestamp, struct tm* tm_time) {
  static int days_four_year = 1461;
  const static unsigned char Days[12] = {31, 28, 31, 30, 31, 30,
                                         31, 31, 30, 31, 30, 31};
  u32 localtime = timestamp + tm_time->__tm_gmtoff;
  tm_time->tm_sec = localtime % 60;
  localtime /= 60;  // min
  tm_time->tm_min = localtime % 60;
  localtime /= 60;  // hour
  tm_time->tm_hour = localtime % 24;
  localtime /= 24;                              // day
  tm_time->tm_wday = (int)(localtime + 4) % 7;  // 1970-1-1 星期四
  localtime += 25567;                           // 1900-1-1 ~ 1970-1-1
  int year = ((localtime / days_four_year) << 2);
  localtime %= days_four_year;
  localtime += ((year + 99) / 100) - ((year + 299) / 400);  // 修正置闰
  int ydays = 365 + (int)(((year % 4 == 0) && (year % 100 != 0)) ||
                          ((year + 300) % 400 == 0));
  while (ydays <= localtime) {
    localtime -= ydays;
    year++;
    ydays = 365 + (int)(((year % 4 == 0) && (year % 100 != 0)) ||
                        ((year + 300) % 400 == 0));
  }
  int leapyear =
      ((year % 4 == 0) && (year % 100 != 0)) || ((year + 300) % 400 == 0);
  tm_time->tm_year = year;
  tm_time->tm_yday = localtime;
  int mon = 0;
  int mdays = Days[mon] + (int)(leapyear && mon == 1);
  while (mdays <= localtime) {
    localtime -= mdays;
    mon++;
    mdays = Days[mon] + (int)(leapyear && mon == 1);
  }
  tm_time->tm_mon = mon;
  tm_time->tm_mday = localtime + 1;
  return tm_time;
}

struct tm* gmtime(u32 timestamp, struct tm* tm_time) {
  tm_time->__tm_gmtoff = 0;
  _gmtime(timestamp, tm_time);
  return tm_time;
}

struct tm* localtime(u32 timestamp, struct tm* tm_time) {
  tm_time->__tm_gmtoff = (LOCAL_TIMEZONE)*3600;
  _gmtime(timestamp, tm_time);
  return tm_time;
}

int strftime(char* s, int max, const char* fmt, struct tm* tm_time) {
  char* p;
  int d, alignz;
  for (p = s; *fmt; fmt++) {
    if (*fmt != '%') {
      *p++ = *fmt;
      continue;
    }
    fmt++;
    if (*fmt == '%') {
      *p++ = *fmt;
      continue;
    }
    alignz = 0;
    switch (*fmt) {
      case 'd':
        d = tm_time->tm_mday;
        alignz = 2;
        goto put_digit;
      case 'H':
        d = tm_time->tm_hour;
        alignz = 2;
        goto put_digit;
      case 'I':
        d = (tm_time->tm_hour + 11) % 12 + 1;
        alignz = 2;
        goto put_digit;
      case 'M':
        d = tm_time->tm_min;
        alignz = 2;
        goto put_digit;
      case 'm':
        d = tm_time->tm_mon + 1;
        alignz = 2;
        goto put_digit;
      case 'S':
        d = tm_time->tm_sec;
        alignz = 2;
        goto put_digit;
      case 'Y':
        d = (tm_time->tm_year + 1900);
        goto put_digit;
      case 'y':
        d = (tm_time->tm_year + 1900) % 100;
        alignz = 2;
        goto put_digit;
      default:
        break;
    }
    continue;
  put_digit:
    p += sprintf(p, "%0*d", alignz, d);
  }
  return (p) - (s);
}
