#include <time.h>
#include <dlfcn.h>

inline double nano_clock() {
  struct timespec time;
  clock_gettime(CLOCK_REALTIME,&time);
  double dSeconds = time.tv_sec;
  double dNanoSeconds = (double)time.tv_nsec/1000000000.0;
  return dSeconds+dNanoSeconds;
}
