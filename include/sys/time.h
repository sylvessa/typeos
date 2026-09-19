#ifndef TYPEOS_SYS_TIME_H
#define TYPEOS_SYS_TIME_H

#include <stdint.h>

struct timeval {
	int64_t tv_sec;
	int64_t tv_usec;
};

int gettimeofday(struct timeval* tv, void* tz);

#endif