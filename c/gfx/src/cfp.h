// custom 16-bit floating point format
#ifndef CFP_H
#define CFP_H

#include <stdint.h>

#define CLZ(x) __builtin_clz(x)
INLINE int ilog2(uint32_t x) {
  return 31 - CLZ(x+1);
}

#define CFP_MANTISSA_BITS  11
#define CFP_MANTISSA_MASK  ((1<<CFP_MANTISSA_BITS)-1)
#define CFP_EXPONENT_BITS  (16-CFP_MANTISSA_BITS)
#define CFP_POINT (CFP_MANTISSA_BITS-1)

static int cfp_e[] = {
  21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,0,0,0,0,0,0,0,0,0,0,0
};


INLINE uint16_t int2cfp(uint32_t n) {
  //int e = ilog2(n) - CFP_POINT;
  //if (e < 0) return n;
  int e = cfp_e[CLZ(n+1)];
  return (e<<CFP_MANTISSA_BITS) | (n>>e);
}

INLINE uint32_t cfp2int(uint16_t n) {
  return ((uint32_t)n&CFP_MANTISSA_MASK)<<(n>>CFP_MANTISSA_BITS);
}

extern uint16_t int2cfp_lut[0x40000];

INLINE uint16_t int2cfpL(uint32_t n) {
  if (LIKELY(n < 0x40000)) return int2cfp_lut[n];
  return int2cfp(n);
}

#endif
