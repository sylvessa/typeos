#ifndef TYPEOS_TIME_H
#define TYPEOS_TIME_H

#include <stdint.h>

typedef int64_t time_t;

struct tm {
	int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year;
	int tm_wday, tm_yday, tm_isdst;
	long tm_gmtoff; // quickjs reads this for the timezone offset
};

time_t time(time_t* t);
struct tm* localtime_r(const time_t* t, struct tm* tm);

#endif