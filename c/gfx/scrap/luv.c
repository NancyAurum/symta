//LUV: HDR and palette-like functionality for truecolor images

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "gfx.h"
#include "luv.h"
#include "common.h"
#include "../../../runtime/flt16.h"

float luv_glut[0x100] = {
#include "luv_glut.h"
};
static uint8_t luv_iglut[0x10000] = {
#include "luv_iglut.h"
};

static float rgb2luv_mtx[] = {
   42.583497053,   0.0,          -42.583497053,
  -24.6989528796, 49.3979057592, -24.6989528796,
  127.7504911591, 74.1937172775, 255.0
};

#if 0
static float luv2rgb_mtx[] = {
   769.4882352941, -442.2252252252,     42.8333333333 + 1563.750437 + 65.4526020138,
     0.0,           884.4504504505,    -85.6666666667 + 176.8900900901,
  -769.4882352941, -442.2252252252, 196647.8333333333 + -242.3426921039
};
#endif

uint32_t rgb2luv(uint32_t rgb) {
  uint32_t ir,ig,ib,u,v;
  float r,g,b,l,*m;
  UNRGB(ir,ig,ib,rgb);
  r = luv_glut[ir];
  g = luv_glut[ig];
  b = luv_glut[ib];
  l = (r+g+b)/3.0f + 0.0001;
  r /= l;
  g /= l;
  b /= l;
  m = rgb2luv_mtx;
  v = (uint32_t)(m[0]*r          + m[2]*b + m[6]);
  u = (uint32_t)(m[3]*r + m[4]*g + m[5]*b + m[7]);
  return LUV(f2h(l),u,v);
}

uint32_t luv2rgb(uint32_t luv) {
  uint32_t il,iu,iv,ir,ig,ib;
  float r,g,b,l,u,v;
  UNLUV(il,iu,iv,luv);
  v = (float)iv;
  u = (float)iu;
  l = h2f(il);
  r =  769.4882352941f*v - 442.2252252252f*u + 1672.0363723471f;
  g =                      884.4504504505f*u + 91.2234234234f;
  b = -769.4882352941f*v - 442.2252252252f*u + 196405.4906412293f;
  ir = luv_iglut[f2h(r*l)];
  ig = luv_iglut[f2h(g*l)];
  ib = luv_iglut[f2h(b*l)];

#if 0
  if (r<0.0f || g<0.0f || b<0.0f) {
    printf("%f,%d,%d:\n", l,iu,iv);
    printf("  %f, %d, %f\n", r*l, ir, r);
    printf("  %f, %d, %f\n", g*l, ig, g);
    printf("  %f, %d, %f\n", b*l, ib, b);
  }
#endif
  return RGB(ir,ig,ib);
}

void rgb2luv_u(int ir, int ig, int ib, float *rl, int *ru, int *rv) {
  uint32_t u,v;
  float r,g,b,l,*m;
  r = luv_glut[ir];
  g = luv_glut[ig];
  b = luv_glut[ib];
  l = (r+g+b)/3.0f + 0.0001;
  r /= l;
  g /= l;
  b /= l;
  m = rgb2luv_mtx;
  *rl = l;
  *rv = (uint32_t)(m[0]*r          + m[2]*b + m[6]);
  *ru = (uint32_t)(m[3]*r + m[4]*g + m[5]*b + m[7]);
}


void luv2rgb_u(float l, int iu, int iv, int *rr, int *rg, int *rb) {
  uint32_t ir,ig,ib;
  float r,g,b,u,v;
  v = (float)iv;
  u = (float)iu;
  r =  769.4882352941f*v - 442.2252252252f*u + 1672.0363723471f;
  g =                      884.4504504505f*u + 91.2234234234f;
  b = -769.4882352941f*v - 442.2252252252f*u + 196405.4906412293f;
  *rr = luv_iglut[f2h(r*l)];
  *rg = luv_iglut[f2h(g*l)];
  *rb = luv_iglut[f2h(b*l)];
}

uint32_t luv_saturate(float s, uint32_t luv) {
  int v = luv&0xFF; 
  int u = (luv>>8)&0xFF; 
  int nv = LUV_WV + (int)((float)(v - LUV_WV)*s);
  int nu = LUV_WU + (int)((float)(u - LUV_WU)*s);
  nv = clamp_byte(nv);
  nu = clamp_byte(nu);
  return (luv&0xFFFF0000)|(nu<<8)|nv;
}

#define ET_TOP_X    0.0f
#define ET_TOP_Y    1.0f
#define ET_LEFT_X   -0.8660254038f
#define ET_LEFT_Y   -0.5f
#define ET_RIGHT_X  0.8660254038f
#define ET_RIGHT_Y  -0.5f
#define THIRD       (1.0f/3.0f)

//triangular metric sin/cos
static INLINE void tsc(float a, float *c, float *s) {
  if (a < THIRD) {
    float m = a/THIRD;
    *c = ET_TOP_X + (ET_LEFT_X - ET_TOP_X)*m;
    *s = ET_TOP_Y + (ET_LEFT_Y - ET_TOP_Y)*m;
  } else if (a < 2.0f*THIRD) {
    float m = (a-THIRD)/THIRD;
    *c = ET_LEFT_X + (ET_RIGHT_X - ET_LEFT_X)*m;
    *s = ET_LEFT_Y + (ET_RIGHT_Y - ET_LEFT_Y)*m;
  } else {
    float m = (a-2.0f*THIRD)/THIRD;
    *c = ET_RIGHT_X + (ET_TOP_X - ET_RIGHT_X)*m;
    *s = ET_RIGHT_Y + (ET_TOP_Y - ET_RIGHT_Y)*m;
  }
}

uint32_t luv_rotate(float a, uint32_t luv) {
  int v, u, nv, nu;
  float dx, dy, cosa, sina;
  a = a - (float)(int)a;
  if (a < 0.0f) a = 1.0f + a;
  v = luv&0xFF; 
  u = (luv>>8)&0xFF;
  dx = (float)(v-LUV_WV);
  dy = (float)(u-LUV_WU);
  tsc(a, &cosa, &sina);
  nv = LUV_WV + (int)(dx*sina - dy*cosa);
  nu = LUV_WU + (int)(dy*sina + dx*cosa);
  nv = clamp_byte(nv);
  nu = clamp_byte(nu);
  return (luv&0xFFFF0000)|(nu<<8)|nv;
}

uint32_t luv_brightness(float s, uint32_t luv) {
  return (f2h(h2f(luv>>16)*s)<<16)|(luv&0xFFFF);
}
