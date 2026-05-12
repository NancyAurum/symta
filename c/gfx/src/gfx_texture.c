#include <math.h>
#include "gfx.h"
#include "common.h"
#include "cfp.h"
#include "ridiv_lut.h"

static void *texturer_lut[1<<10];

#include "br_lut.h"

#define GFX_BLIT_ARGS gfx_t *RESTRICT dst, gfx_t *RESTRICT src \
                       , int x, int y, int length \
                       , double z, double ez \
                       , double u, double eu \
                       , double v, double ev

typedef void blitter_fn(GFX_BLIT_ARGS);

#define SC *s
#define DC *d

#define TEXTURE_BLIT 1
#include "gfx_blitters.h"

void _gfx_init_texturers() {
  _gfx_init_blitters(texturer_lut);
}

typedef struct tlerp {
  double x;
  double z;
  double u;
  double v;
  double zi;
  double xi;
  double ui;
  double vi;
} tlerp;

static void tlerp_init(tlerp *l
                      , double first_step, double steps
                      , double sx, double ex
                      , double sz, double ez
                      , double su, double eu
                      , double sv, double ev)
{
  l->xi = (ex - sx) / steps;
  l->zi = (ez - sz) / steps;
  l->ui = (eu - su) / steps;
  l->vi = (ev - sv) / steps;
  l->x = sx + first_step*l->xi;
  l->z = sz + first_step*l->zi;
  l->u = su + first_step*l->ui;
  l->v = sv + first_step*l->vi;
}

static void tlerp_advance(tlerp *l) {
  l->x += l->xi;
  l->z += l->zi;
  l->u += l->ui;
  l->v += l->vi;
}

#define TRIANGLE_ROW(a,b) do { \
  int x1 = (int)a.x; \
  int x2 = (int)b.x; \
  double sz = a.z; \
  double ez = b.z; \
  double su = a.u; \
  double eu = b.u; \
  double sv = a.v; \
  double ev = b.v; \
  if (x1 < x2) \
       texture_row(gfx, src, x1, y, x2-x1, sz, ez, su, eu, sv, ev); \
  else texture_row(gfx, src, x2, y, x1-x2, ez, sz, eu, su, ev, sv); \
} while (0)

void gfx_textured(gfx_t * RESTRICT gfx, gfx_t * RESTRICT src
        , int ax, int ay, float az
        , int bx, int by, float bz
        , int cx, int cy, float cz
        , float au, float av
        , float bu, float bv
        , float cu, float cv)
{
  int beg_y, cen_y, end_y;
  int y, e;
  int dw = gfx->w, dh = gfx->h;
  tlerp l, r;
  
  if(ax < 0 && bx < 0 && cx < 0) return;
  if(ax >= dw && bx >= dw && cx >= dw) return;
  if(ax == bx && ax == cx) return;

  blitter_fn *texture_row = (blitter_fn*)texturer_lut[pbl->bflags&0x1ff];

  if (ay > by) {
    SWAP( ax,bx);
    SWAP( ay,by);
    SWAPD(az,bz);
    SWAPD(au,bu);
    SWAPD(av,bv);
  }

  if (ay > cy) {
    SWAP( ax,cx);
    SWAP( ay,cy);
    SWAPD(az,cz);
    SWAPD(au,cu);
    SWAPD(av,cv);
  }

  if (by > cy) {
    SWAP( bx,cx);
    SWAP( by,cy);
    SWAPD(bz,cz);
    SWAPD(bu,cu);
    SWAPD(bv,cv);
  }

  beg_y = ay;
  cen_y = by;
  end_y = cy;

  if(end_y == beg_y || end_y < 0 || beg_y >= dh) return;

  au *= az;
  av *= az;
  bu *= bz;
  bv *= bz;
  cu *= cz;
  cv *= cz;
  /*az = 1.0/az;
  bz = 1.0/bz;
  cz = 1.0/cz;*/

  if (beg_y < 0) {
    tlerp_init(&r,   -beg_y, end_y-beg_y, ax, cx, az, cz, au, cu, av, cv);
    y = 0;
  } else {
    tlerp_init(&r,        0, end_y-beg_y, ax, cx, az, cz, au, cu, av, cv);
    y = beg_y;
  }

  if (y < cen_y) {
    if (beg_y < 0) {
      tlerp_init(&l, -beg_y, cen_y-beg_y, ax, bx, az, bz, au, bu, av, bv);
    } else {
      tlerp_init(&l,      0, cen_y-beg_y, ax, bx, az, bz, au, bu, av, bv);
    }

    if (cen_y > dh) e = dh;
    else e = cen_y;

    for (; y < e; ++y) {
      TRIANGLE_ROW(l,r);
      tlerp_advance(&l);
      tlerp_advance(&r);
    }
  }

  if(cen_y < end_y) {
    tlerp_init(&l, y-cen_y, end_y-cen_y, bx, cx, bz, cz, bu, cu, bv, cv);
    if (end_y > dh) end_y = dh;
    
    for (; y < end_y; ++y) {
      TRIANGLE_ROW(l,r);
      tlerp_advance(&l);
      tlerp_advance(&r);
    }
  }
}

static float dist(int ax, int ay, int bx, int by) {
  int x = ax-bx;
  int y = ay-by;
  return sqrtf((float)(x*x + y*y));
}

void gfx_quad(gfx_t *dst, gfx_t *src, int *quad) {
  int *q = quad;
  float top = dist(q[0],q[1],q[2],q[3]);
  float bot = dist(q[4],q[5],q[6],q[7]);
  float lft = dist(q[0],q[1],q[4],q[5]);
  float rgt = dist(q[2],q[3],q[6],q[7]);
  float az = 1.0;
  float bz = 1.0;
  float cz = 1.0;
  float dz = 1.0;

  if (top<bot) {
    az *= top/bot;
    bz *= top/bot;
  } else {
    cz *= bot/top;
    dz *= bot/top;
  }

  if (lft<rgt) {
    az *= lft/rgt;
    cz *= lft/rgt;
  } else {
    bz *= rgt/lft;
    dz *= rgt/lft;
  }

  gfx_textured(dst, src
              , q[0],q[1],az, q[2],q[3],bz, q[4],q[5],cz
              , 0.0,0.0,      1.0,0.0,      0.0,1.0);
  gfx_textured(dst, src
              , q[2],q[3],bz, q[4],q[5],cz, q[6],q[7],dz
              , 1.0,0.0,      0.0,1.0,      1.0,1.0);

  //gfx_triangle(dst, 0xFF0000, q[0],q[1], q[2],q[3], q[4],q[5]);
  //gfx_triangle(dst, 0x00FF00, q[2],q[3], q[4],q[5], q[6],q[7]);

}
