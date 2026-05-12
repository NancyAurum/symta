int sx = pbl->bx;
int sy = pbl->by;
int esx = pbl->bw;
int esy = pbl->bh;
int xi=1, yi=1;

#include "gfx_blit_head.h"

if (UNLIKELY(x < 0)) {
  x = -x;
  xi = -xi;
}

if (UNLIKELY(y < 0)) {
  y = -y;
  yi = -yi;
}


#define RLE_BLIT_BEGIN(sn,lb)             \
  uint32_t *RESTRICT dd = dst->data + y*dst->w + x; \
  int dww = yi*dst->w;                    \
  uint16_t *RESTRICT*RESTRICT yr = src->rle+sy; \
  uint16_t *RESTRICT*RESTRICT eyr = src->rle+esy; \
while (LIKELY(yr < eyr)) {                \
  uint32_t *RESTRICT d = dd;              \
  uint32_t *ed = d + (esx-sx)*sn;         \
  int rx = 0;                             \
  int run;                                \
  uint16_t *RESTRICT p = *yr++;           \
  uint32_t *RESTRICT s = src->data + *(uint32_t*)p; \
  for (p += 2; ; p++) {                   \
    if (rx + *p > sx) {                   \
      if (UNLIKELY(rx >= esx)) goto lb;   \
      rx += *p++;                         \
      d += sn*(rx-sx);                    \
      run = *p++;                         \
      break;                              \
    }                                     \
    rx += *p;                             \
    p++;                                  \
    if (rx + *p > sx) {                   \
      if (UNLIKELY(rx >= esx)) goto lb;   \
      s += sx-rx;                         \
      run = *p++ - (sx-rx);               \
      break;                              \
    }                                     \
    rx += *p;                             \
    s += *p;                              \
  }                                       \
  while (LIKELY((ed-d)*sn > 0)) {         \
    uint32_t *end = d + run*sn;           \
    if (UNLIKELY((ed-end)*sn < 0))        \
      end = ed;                           \


/*
      while (LIKELY(d != end)) {


        s++;                    \
        d += sn;                \
      }                         \
*/


#define RLE_BLIT_END(sn,lb)   \
    d += sn * *p;             \
    p++;                      \
    run = *p++;               \
  }                           \
  lb:;                        \
  dd += dww;                  \
}


#define BEGIN_BLIT \
  uint32_t *RESTRICT d = dst->data + y*dst->w + x; \
  int w = esx-sx; \
  uint32_t *RESTRICT s = src->data + sy*src->w + sx; \
  uint32_t *RESTRICT ess = src->data + esy*src->w; \
  int sww = src->w - w; \
  int dww = dst->w*yi - xi*w; \
  while (LIKELY(s < ess)) { \
    uint32_t *RESTRICT es = s + w; \
    while (LIKELY(s < es)) {


#define END_BLIT \
      d += xi; \
      s++; \
    } \
    d += dww; \
    s += sww; \
  }


#ifdef B_RLE
do {
  if (xi > 0) {
RLE_BLIT_BEGIN(1,next_rle_line)
#if 1 //RLE unroll
int l = end-d;
end -= l&0x7;
while (LIKELY(d < end)) {
#include "gfx_blit_body.h"
  s++;
  d++;
#include "gfx_blit_body.h"
  s++;
  d++;
#include "gfx_blit_body.h"
  s++;
  d++;
#include "gfx_blit_body.h"
  s++;
  d++;
#include "gfx_blit_body.h"
  s++;
  d++;
#include "gfx_blit_body.h"
  s++;
  d++;
#include "gfx_blit_body.h"
  s++;
  d++;
#include "gfx_blit_body.h"
  s++;
  d++;
}
end += l&0x7;
while (LIKELY(d < end)) {
#include "gfx_blit_body.h"
  s++;
  d++;
}
#else //RLE unroll
while (LIKELY(d < end)) {
#include "gfx_blit_body.h"
  s++;
  d++;
}
#endif //RLE unroll
RLE_BLIT_END(1,next_rle_line)
  } else {
RLE_BLIT_BEGIN(-1,next_rle_line2)
while (LIKELY(d > end)) {
#include "gfx_blit_body.h"
  s++;
  d--;
}
RLE_BLIT_END(-1,next_rle_line2)
  }
} while(0);
#else //B_RLE
#if 1
  uint32_t *RESTRICT d = dst->data + y*dst->w + x;
  int w = esx-sx;
  uint32_t *RESTRICT s = src->data + sy*src->w + sx;
  uint32_t *RESTRICT ess = src->data + esy*src->w;
  int sww = src->w - w;
  int dww = dst->w*yi - xi*w;
  while (LIKELY(s < ess)) {
    uint32_t *RESTRICT es = s + (w&~0x7);
    while (LIKELY(s < es)) {
#include "gfx_blit_body.h"
      d += xi;
      s++;
#include "gfx_blit_body.h"
      d += xi;
      s++;
#include "gfx_blit_body.h"
      d += xi;
      s++;
#include "gfx_blit_body.h"
      d += xi;
      s++;
#include "gfx_blit_body.h"
      d += xi;
      s++;
#include "gfx_blit_body.h"
      d += xi;
      s++;
#include "gfx_blit_body.h"
      d += xi;
      s++;
#include "gfx_blit_body.h"
      d += xi;
      s++;
    }
    es = s + (w&0x7);
    while (LIKELY(s < es)) {
#include "gfx_blit_body.h"
      d += xi;
      s++;
    }
    d += dww;
    s += sww;
  }
#else
BEGIN_BLIT
#include "gfx_blit_body.h"
END_BLIT
#endif
#endif //B_RLE

