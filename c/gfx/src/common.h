#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define SWAP(x,y) do {int t_ = x; x = y; y = t_;} while(0)
#define SWAPD(x,y) do {double t_ = x; x = y; y = t_;} while(0)

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))

#define LUMA8(r,g,b) ((1224*(r) + 2404*(g) + 466*(b))>>12)

//declaring a pointer with RESTRICT, we guarantee compiler,
//that no other pointer will be used to modify the pointed data.
//so compiler could keep more stuff in memory,
//instead of immediately writing back.
//#define RESTRICT __restrict__
#define RESTRICT restrict
//#define RESTRICT 

//inline forces compiler to expand the function into callee.
#define INLINE static __attribute__((always_inline)) inline

#define NOINLINE __attribute__ ((noinline))

#define ALIGN(N) __attribute__ ((aligned(N)))

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

//#define LIKELY(x) (x)
//#define UNLIKELY(x) (x)


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

//following implementations are known to compiler into faster x86 code by gcc
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


#ifndef WINDOWS
#define X86_SSE 1
#endif

#ifdef X86_SSE
//#include <emmintrin.h>
//#include <smmintrin.h>
//#include <xmmintrin.h>
#include <immintrin.h>

ALIGN(16) static uint8_t imask_[] = {
    12+0, 0xff, 0xff, 0xff
  , 8+1, 0xff, 0xff, 0xff
  , 4+2, 0xff, 0xff, 0xff
  ,   3, 0xff, 0xff, 0xff
};

ALIGN(16) static uint8_t omask_[] = {
    0,4,8,0xff //12
  ,0xff, 0xff, 0xff, 0xff
  ,0xff, 0xff, 0xff, 0xff
  ,0xff, 0xff, 0xff, 0xff
};
#define sse_imsk (*(__m128i*RESTRICT)imask_)
#define sse_omsk (*(__m128i*RESTRICT)omask_)
#endif



// blit settings
typedef struct {
  uint32_t bflags;       // blit flags
  float saturation;      // blitting param for saturation change
  int bx; int by; int bw; int bh; //source blitting rect
  float light_energy;    // energy for light
  uint32_t light_color;  // color filter for light
  int quad[8];           // scale/transform quadrilateral
  uint32_t *recolored; //premade recolor
  uint32_t brecolored[256]; //premade recolor

  uint8_t *RESTRICT r_br;
  uint8_t *RESTRICT g_br;
  uint8_t *RESTRICT b_br;
  uint8_t *RESTRICT a_br;

  uint8_t br_lut[256];
  uint8_t r_br_lut[256];
  uint8_t g_br_lut[256];
  uint8_t b_br_lut[256];
  uint8_t a_br_lut[256];

  int prev_br;

  float prev_c_br;
  float prev_c_r;
  float prev_c_g;
  float prev_c_b;

  int prev_fade;

  uint32_t prev_stencil;
  uint8_t r_st_lut[256];
  uint8_t g_st_lut[256];
  uint8_t b_st_lut[256];
  uint8_t a_st_lut[256];
} blit_t;

extern blit_t *volatile pbl;

#endif
