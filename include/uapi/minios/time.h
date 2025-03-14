#pragma once

#include <stddef.h>

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

typedef ssize_t clock_t;

#define CLOCKS_PER_SEC 1000

#define UTC 0
#define LOCAL_TIMEZONE +8
