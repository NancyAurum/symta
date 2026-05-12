#ifndef NCU_CLAMP_H
#define NCU_CLAMP_H

#include <stdint.h>

#if 0
//from https://codereview.stackexchange.com/questions/6502/fastest-way-to-clamp-an-integer-to-the-range-0-255
INLINE uint8_t clamp_byte(int32_t n) {
  n &= -(n >= 0);
  return n | ((255 - n) >> 31);
}
#endif

#if 0
//from http://guihaire.com/code/?p=54
//obviously requires proper signed shifts
INLINE uint8_t clamp_byte(int32_t n) {
  return (((255-n)>>31) | (n & ~(n>>31)));
}
#endif

//following implementations are known to compile into faster x86 code by gcc
INLINE uint8_t clamp_byte(int32_t n) {
  n = n > 255 ? 255 : n;
  return n < 0 ? 0 : n;
}

//clamps byte to 255, if it is above
INLINE uint8_t clamp_byte255(int32_t n) {
  return n > 255 ? 255 : n;
}

//clamps byte to 0, if it is below
INLINE uint8_t clamp_byte0(int32_t n) {
  return n < 0 ? 0 : n;
}

INLINE int32_t clamp(int32_t min, int32_t max, int32_t n) {
  n = n > max ? max : n;
  return n < min ? min : n;
}

INLINE int32_t isign(int32_t x) {
  return (int32_t)(((uint32_t)-x >> 31) - ((uint32_t)x >> 31));
}

#endif
