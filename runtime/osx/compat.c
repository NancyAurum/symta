#include "compat.h"

// OS X does not have clock_gettime, define it through clock_get_time
#include <mach/clock.h>
#include <mach/mach.h>

int cmt_clock_gettime(int clk_id, struct timespec *ts) {
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
  return 0;
}
