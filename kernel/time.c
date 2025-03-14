#include <minios/time.h>

// 计算自1970-1-1 00:00:00 UTC 的秒数(时间戳) 全是技巧的Gauss算法, copy自linux
u32 mktime(struct tm* time) {
    unsigned int mon = time->tm_mon + 1, year = time->tm_year + 1900, day = time->tm_mday,
                 hour = time->tm_hour, min = time->tm_min, sec = time->tm_sec;

    /* 1..12 -> 11,12,1..10 */
    if (0 >= (int)(mon -= 2)) {
        mon += 12; /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return ((((unsigned long)(year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day) +
              year * 365 - 719499) *
                 24 +
             hour /* now have hours */
             ) * 60 +
            min /* now have minutes */
            ) * 60 +
           sec - time->__tm_gmtoff; /* finally seconds */
}

static struct tm* _gmtime(u32 timestamp, struct tm* tm_time) {
    static int days_four_year = 1461;
    static const unsigned char Days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    u32 localtime = timestamp + tm_time->__tm_gmtoff;
    tm_time->tm_sec = localtime % 60;
    localtime /= 60; // min
    tm_time->tm_min = localtime % 60;
    localtime /= 60; // hour
    tm_time->tm_hour = localtime % 24;
    localtime /= 24;                             // day
    tm_time->tm_wday = (int)(localtime + 4) % 7; // 1970-1-1 星期四
    localtime += 25567;                          // 1900-1-1 ~ 1970-1-1
    int year = ((localtime / days_four_year) << 2);
    localtime %= days_four_year;
    localtime += ((year + 99) / 100) - ((year + 299) / 400); // 修正置闰
    u32 ydays = 365 + (int)(((year % 4 == 0) && (year % 100 != 0)) || ((year + 300) % 400 == 0));
    while (ydays <= localtime) {
        localtime -= ydays;
        year++;
        ydays = 365 + (int)(((year % 4 == 0) && (year % 100 != 0)) || ((year + 300) % 400 == 0));
    }
    int leapyear = ((year % 4 == 0) && (year % 100 != 0)) || ((year + 300) % 400 == 0);
    tm_time->tm_year = year;
    tm_time->tm_yday = localtime;
    int mon = 0;
    u32 mdays = Days[mon] + (int)(leapyear && mon == 1);
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
    tm_time->__tm_gmtoff = LOCAL_TIMEZONE * 3600;
    _gmtime(timestamp, tm_time);
    return tm_time;
}

int kern_get_time(struct tm* time) {
    localtime(current_timestamp, time);
    return 0;
}
