/* ISO C `broken-down time' structure.  */
#ifndef TIME_H
#define TIME_H
#include <kernel/type.h>
struct tm {
    int tm_sec;   /* Seconds.	[0-60] (1 leap second) */
    int tm_min;   /* Minutes.	[0-59] */
    int tm_hour;  /* Hours.	[0-23] */
    int tm_mday;  /* Day.		[1-31] */
    int tm_mon;   /* Month.	[0-11] */
    int tm_year;  /* Year	- 1900.  */
    int tm_wday;  /* Day of week.	[0-6] */
    int tm_yday;  /* Days in year.[0-365]	*/
    int tm_isdst; /* DST.		[-1/0/1]*/

    long int __tm_gmtoff;  /* Seconds east of UTC.  */
    const char* __tm_zone; /* Timezone abbreviation.  */
};
#define UTC 0
#define LOCAL_TIMEZONE +8 // timestamp use UTC， localtime UTC+8
#define RTC_TIMEZONE UTC  // read cmos rtc as UTC
extern u32 current_timestamp;
u32 mktime(struct tm* time);
struct tm* gmtime(u32 timestamp, struct tm* tm_time);
struct tm* localtime(u32 timestamp, struct tm* tm_time);
u32 get_init_rtc_timestamp();
#endif
