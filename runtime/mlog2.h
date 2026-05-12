//Calculates log2 for interger constants at compile time
#ifndef MLOG2_H
#define MLOG2_H

#include "stdint.h"

#define mlog2N(D, N) \
  ((     (uint64_t)(N) >= ((uint64_t)1 << (D - 1)) \
      && (uint64_t)(N) <  ((uint64_t)1 << D)) \
   ? D : -1)

#define mlog2(N)       \
  ((N) == 0 ? 1        \
    : (31              \
    + mlog2N( 1, N)    \
    + mlog2N( 2, N)    \
    + mlog2N( 3, N)    \
    + mlog2N( 4, N)    \
    + mlog2N( 5, N)    \
    + mlog2N( 6, N)    \
    + mlog2N( 7, N)    \
    + mlog2N( 8, N)    \
    + mlog2N( 9, N)    \
    + mlog2N(10, N)    \
    + mlog2N(11, N)    \
    + mlog2N(12, N)    \
    + mlog2N(13, N)    \
    + mlog2N(14, N)    \
    + mlog2N(15, N)    \
    + mlog2N(16, N)    \
    + mlog2N(17, N)    \
    + mlog2N(18, N)    \
    + mlog2N(19, N)    \
    + mlog2N(20, N)    \
    + mlog2N(21, N)    \
    + mlog2N(22, N)    \
    + mlog2N(23, N)    \
    + mlog2N(24, N)    \
    + mlog2N(25, N)    \
    + mlog2N(26, N)    \
    + mlog2N(27, N)    \
    + mlog2N(28, N)    \
    + mlog2N(29, N)    \
    + mlog2N(30, N)    \
    + mlog2N(31, N)    \
    + mlog2N(32, N)    \
    )                  \
  )

#endif