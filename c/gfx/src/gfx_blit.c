#include <math.h>
#include "gfx.h"
#include "common.h"
#include "cfp.h"
#include "ridiv_lut.h"

//#define SRGB_GAMMA 2.0
#define SRGB_GAMMA 2.2
//#define SRGB_GAMMA 2.8


static void *blitter_lut[1<<10];

//default gamma lookup table
gfx_gamma_t dgam;

#include "br_lut.h"

#define GFX_BLIT_ARGS gfx_t * RESTRICT dst, int x, int y, gfx_t * RESTRICT src


//source and destination colors
#define SC *s
#define DC *d

#include "gfx_blitters.h"

void _gfx_init_texturers();

void gfx_gamma_init(gfx_gamma_t *g, double gamma) {
  int i, ia;
  double igamma = 1.0/gamma;
  gfx_gamma_t *RESTRICT pg = g;
  pg->value = gamma;
  for (ia = 0; ia < 256; ia++) {
    float fa = ((float)ia) / 255;
    for (i = 0; i < 256; i++) {
      pg->ab_lut[ia][i] = (uint32_t)(pow((float)i,gamma)*fa);
      pg->fab_lut[ia][i] = pg->ab_lut[ia][i]>>8;
    }
  }
  for (i = 0; i < 0x100; i++) {
    pg->glut[i] = (uint32_t)pow((double)i,gamma);
  }
  for (i = 0; i < 0x20000; i++) {
    int v = cfp2int((uint16_t)i);
    pg->iglut[i] = clamp_byte((int)pow((double)v,igamma));
  }
}

uint16_t int2cfp_lut[0x40000];

void init_int2cfp_lut() {
  int i;
  for (i = 0; i < 0x40000; i++) {
    int2cfp_lut[i] = int2cfp((uint32_t)i);
  }
}

void _gfx_init_tables() {
  int i;
  init_int2cfp_lut();
  gfx_gamma_init(&dgam, SRGB_GAMMA);
  _gfx_init_blitters(blitter_lut);
  _gfx_init_texturers();
}


typedef void blitter_fn(GFX_BLIT_ARGS);

NOINLINE void _gfx_blit(gfx_t * RESTRICT gfx
                      ,int x, int y, gfx_t * RESTRICT src) {
  gfx_t * RESTRICT dst = gfx;
  int cx = 0, cy = 0, cw = dst->w, ch = dst->h; //canvas x,y,w,h
  int ex, ey; //end coords in destination
  int sw = src->w;
  int sh = src->h;
  uint32_t bfs = pbl->bflags;
  int sx, sy, w, h; //source rect
  int esx, esy; //end coords in source
  int flip_x = bfs&GFX_BF_FLIP_X;
  int flip_y = UNLIKELY(bfs&GFX_BF_FLIP_Y);
  pbl->bflags = 0;

  //default source rect
  sx = 0;
  sy = 0;
  w = src->w;
  h = src->h;

  x += flip_x ? -src->x : src->x;
  if (x >= cw) return;

  y += flip_y ? -src->y : src->y;
  if (y >= ch) return;

  if (UNLIKELY(bfs&GFX_BF_RECT)) {
    sx = pbl->bx;
    sy = pbl->by;
    w = pbl->bw;
    h = pbl->bh;
  }

  if (UNLIKELY(dst->scissor)) { //FIXME: should it go before the flips?
    int dsx = dst->scissor->rect[0];
    int dsy = dst->scissor->rect[1];
    int dex = dsx + dst->scissor->rect[2];
    int dey = dsy + dst->scissor->rect[3];
    int ssx = x;
    int ssy = y;
    int sex = ssx + src->w;
    int sey = ssy + src->h;
    int dx0 = dsx-ssx;
    int dy0 = dsy-ssy;
    int dx1 = sex-dex;
    int dy1 = sey-dey;
    int rx = 0;
    int ry = 0;
    if (dx0 > 0) {
      rx = dx0;
      w -= dx0;
    }
    if (dy0 > 0) {
      ry = dy0;
      h -= dy0;
    }
    if (dx1 > 0) w -= dx1;
    if (dy1 > 0) h -= dy1;
    sx += rx;
    sy += ry;
    x += rx;
    y += ry;
    if (w <= 0 || h <= 0) return; //could happen
  }

  if (UNLIKELY((src->flags&GFX_F_OCT)==GFX_OCT_UNKNOWN)) {
    gfx_determine_opacity(src);
  }

  if (UNLIKELY(bfs&GFX_BF_QUAD)) {
    int i;
    int xx = src->x;
    int yy = src->y;
    src->x = 0;
    src->y = 0;
    if (!(bfs&GFX_BF_ADDITIVE)) {
      bfs |= src->flags&(GFX_OCT_BLEND|GFX_F_GBLEND);
    }
    pbl->bflags = bfs&~(GFX_BF_QUAD);
    gfx_t *tmp = src;
    if (src->flags&GFX_BF_RLE || pbl->bflags&GFX_BF_RECT) {
      uint32_t bfs = pbl->bflags&~GFX_BF_RECT;
      tmp = new_gfx(w,h);
      _gfx_clear(tmp, 0xFF000000);
      _gfx_blit(tmp, 0, 0, src);
      pbl->bflags = bfs;
      tmp->flags = src->flags&~GFX_BF_RLE;
      tmp->rmap = src->rmap;
      tmp->recolors = src->recolors;
      tmp->gamma = src->gamma;
    }
    for (i = 0; i < 4; i++) {
      pbl->quad[i*2] += x;
      pbl->quad[i*2+1] += y;
    }
    gfx_quad(dst, tmp, pbl->quad);
    if (tmp != src) {
      tmp->rmap = 0;
      tmp->gamma = &dgam;
      _free_gfx(tmp);
    }
    src->x = xx;
    src->y = yy;
    pbl->bflags = 0;
    return;
  }

  if (UNLIKELY(sx < 0)) {
    w += sx;
    sx = 0;
  }
  if (sx + w >= sw) w = sw - sx;
  ex = x + w;
  if (UNLIKELY(ex <= cx)) return;

  if (sy < 0) {
    h += sy;
    sy = 0;
  }
  if (sy + h >= sh) h = sh - sy;
  ey = y + h;
  if (UNLIKELY(ey <= cy)) return;


  esx = sx+w;

  if (UNLIKELY(x < cx)) {
    if (!flip_x) sx += cx-x;
    x = cx;
  }
  if (UNLIKELY(ex > cw)) {
    esx -= ex-cw;
    ex = cw;
  }

  if (flip_x) {
    sx = sx + w - esx;
    esx = sx + ex-x;
    x = 1 - ex;
  }
  esy = sy+h;

  if (UNLIKELY(y < cy)) {
    if (!flip_y) sy += cy-y;
    y = cy;
  }
  if (UNLIKELY(ey > ch)) {
    esy -= ey-ch;
    ey = ch;
  }

  if (UNLIKELY(flip_y)) {
    sy = sy + h - esy;
    esy = sy + ey-y;
    y = 1 - ey;
  }
  if (bfs&GFX_BF_ADDITIVE) {
    bfs |= src->flags&(GFX_F_RLE|GFX_F_GBLEND);
  } else {
    bfs |= src->flags&(GFX_F_RLE|GFX_F_GBLEND|GFX_OCT_BLEND);
  }

  pbl->bx = sx;
  pbl->by = sy;
  pbl->bw = esx;
  pbl->bh = esy;

  ((blitter_fn*)blitter_lut[bfs&0x1ff])(gfx, x, y, src);
}
