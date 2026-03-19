

#ifndef AVUTIL_TIME_H
#define AVUTIL_TIME_H

#include <stdint.h>


int64_t av_gettime(void);


int64_t av_gettime_relative(void);


int av_gettime_relative_is_monotonic(void);


int av_usleep(unsigned usec);

#endif 
