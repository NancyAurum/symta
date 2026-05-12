if ((uint32_t)y >= (uint32_t)dst->h) return;

double sw = src->w;
double sh = src->h;
double l = (double)length + 0.000000001;
double zi = (ez-z)/l;
double ui = (eu-u)/l;
double vi = (ev-v)/l;

if (UNLIKELY(x < 0)) {
  z -= x*zi;
  u -= x*ui;
  v -= x*vi;
  length = x + length;
  x = 0;
}

int dw = dst->w;
if (UNLIKELY(x+length > dw)) length = dw - x;

#include "gfx_blit_head.h"

uint32_t *ps = src->data;
uint32_t *d = dst->data + y*dw + x;
uint32_t *d_end = d + length;
double zz = 1.0/z;
for (; d < d_end; d++) {
  uint32_t c;
  uint32_t sx = (uint32_t)(int)(sw*u*zz);
  uint32_t sy = (uint32_t)(int)(sh*v*zz);
  z += zi;
  zz = 1.0/z; // do it in advance to keep FPU bussy
  u += ui;
  v += vi;
  if (UNLIKELY(sx >= sw)) sx = sw-1;
  if (UNLIKELY(sy >= sh)) sy = sh-1;
  uint32_t *s = src->data + sy*src->w + sx;

  do {
#include "gfx_blit_body.h"
  } while(0);
}

