#ifndef SYMTA_COMPAT_H_
#define SYMTA_COMPAT_H_

#include <unistd.h>
#include <time.h>

//#define CLOCK_REALTIME 0
int cmt_clock_gettime(int clk_id, struct timespec *ts);

#endif
