#ifndef SYMTA_COMPAT_H_
#define SYMTA_COMPAT_H_

#include <sys/types.h>
#include <sys/stat.h>

char *realpath(const char *path, char *resolved_path);

//check for mingw w64, which has different library from usual mingw
struct timespec;
int cmt_clock_gettime(int X, struct timespec *ts);

#endif
