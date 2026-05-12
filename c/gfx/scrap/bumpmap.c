#include <math.h>
#include "gfx.h"
#include "luv.h"
#include "common.h"


#define SWAP(x,y) do {int t_ = x; x = y; y = t_;} while(0)

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))


void *ffi_alloc_(int size) {
  return malloc(size);
}

void ffi_free_(void *ptr) {
  free(ptr);
}

void ffi_memset_(void *ptr, int value, uint32_t size) {
  memset(ptr, value, size);
}

//opacity types
#define OCT_UNKNOWN      0
#define OCT_OPAQUE       1
#define OCT_TRANS        2
#define OCT_ALPHA        3

gfx_t *new_gfx(uint32_t w, uint32_t h) {
  gfx_t *gfx = (gfx_t*)malloc(sizeof(gfx_t));
  if (!gfx) return 0;
  gfx->data = (uint32_t*)malloc(w*h*sizeof(uint32_t));
  if (!gfx->data) {
    free(gfx);
    return 0;
  }
  gfx->w = w;
  gfx->h = h;
  gfx->cmap = 0;
  gfx->x = 0;
  gfx->y = 0;
  gfx->flags = 0;
  gfx->bflags = 0;
  gfx->recolor_map = 0;
  gfx->blit_bright = 0;
  gfx->bumpmap = 0;
  gfx->opacity = OCT_UNKNOWN;
  return gfx;
}

void free_gfx(gfx_t *gfx) {
  if (gfx->cmap) free(gfx->cmap);
  if (gfx->bumpmap) free(gfx->bumpmap);
  free(gfx->data);
  free(gfx);
}

void gfx_resize(gfx_t *gfx, uint32_t w, uint32_t h) {
  if (gfx->bumpmap) {
    free(gfx->bumpmap);
    gfx->bumpmap = 0;
  }
  free(gfx->data);
  gfx->data = (uint32_t*)malloc(w*h*sizeof(uint32_t));
  gfx->w = w;
  gfx->h = h;
}

void gfx_switch_luv(gfx_t *gfx) {
  if (gfx->flags&GFX_FLAG_LUV) {
    gfx->flags &= ~(uint32_t)GFX_FLAG_LUV;
  } else {
    gfx->flags |= (uint32_t)GFX_FLAG_LUV;
  }
}

void gfx_to_luv(gfx_t *gfx) {
  uint32_t *p = gfx->data;
  uint32_t *end = gfx->data + gfx->w*gfx->h;
  if (gfx->flags&GFX_FLAG_LUV) return;
  gfx_switch_luv(gfx);

  while (p < end) {
    *p = rgb2luv(*p&0xFFFFFF) + (*p&0xFF000000);
    p++;
  }
}

void gfx_to_rgb(gfx_t *gfx) {
  uint32_t *p = gfx->data;
  uint32_t *end = gfx->data + gfx->w*gfx->h;
  unless (gfx->flags&GFX_FLAG_LUV) return;
  gfx_switch_luv(gfx);

  while (p < end) {
    *p = luv2rgb(*p&0xFFFFFF) + (*p&0xFF000000);
    p++;
  }
}

uint32_t gfx_w(gfx_t *gfx) {
  return gfx->w;
}

uint32_t gfx_h(gfx_t *gfx) {
  return gfx->h;
}

uint32_t *gfx_pixels(gfx_t *gfx) {
  return gfx->data;
}

void *gfx_enable_cmap(gfx_t *gfx) {
  unless (gfx->cmap) {
    gfx->cmap = malloc(sizeof(uint32_t)*GFX_CMAP_SIZE);
  }
  return gfx->cmap;
}

uint32_t *gfx_cmap(gfx_t *gfx) {
  return gfx->cmap;
}

void gfx_set_cmap(gfx_t *gfx, uint32_t *cmap) {
  unless (gfx->cmap) {
    gfx->cmap = malloc(sizeof(uint32_t)*GFX_CMAP_SIZE);
  }
  memcpy(gfx->cmap, cmap, sizeof(uint32_t)*GFX_CMAP_SIZE);
}

int gfx_x(gfx_t *gfx) {
  return gfx->x;
}

int gfx_y(gfx_t *gfx) {
  return gfx->y;
}

void gfx_set_xy(gfx_t *gfx, int x, int y) {
  gfx->x = x;
  gfx->y = y;
}

uint32_t gfx_get(gfx_t *gfx, int x, int y) {
  if ((uint32_t)x >= (uint32_t)gfx->w) return 0;
  if ((uint32_t)y >= (uint32_t)gfx->h) return 0;
  return gfx->data[gfx->w*y+x];
}

void gfx_set(gfx_t *gfx, int x,int y, uint32_t color) {
  if ((uint32_t)x >= (uint32_t)gfx->w) return;
  if ((uint32_t)y >= (uint32_t)gfx->h) return;
  gfx->data[gfx->w*y+x] = color;
}

void gfx_clear(gfx_t *gfx, uint32_t color) {
  int x, y;
  for (y = 0; y < gfx->h; y++) {
    for (x = 0; x < gfx->w; x++) {
      gfx->data[gfx->w*y+x] = color;
    }
  }
}

static void determine_opacity(gfx_t *gfx) {
  int x, y;
  gfx->opacity = OCT_OPAQUE;
  for (y = 0; y < gfx->h; y++) {
    for (x = 0; x < gfx->w; x++) {
      uint32_t alpha = gfx->data[gfx->w*y+x]&0xFF000000;
      if (alpha) {
        if (alpha != 0xFF000000) {
          gfx->opacity = OCT_ALPHA;
          return;
        }
        gfx->opacity = OCT_TRANS;
      }
    }
  }
}

static void gfx_hline(gfx_t *gfx, uint32_t color, int x, int y, int length) {
  int i, e;
  int w = gfx->w;
  int h = gfx->h;
  uint32_t *d = gfx->data;
  if ((uint32_t)y >= (uint32_t)h) return;
  if (x < 0) {
    length = x + length;
    x = 0;
  }
  if (x+length > w) length = w - x;
  i = y*w + x;
  e = i + length;
  for (; i < e; i++) {
    d[i] = color;
  }
}

static void gfx_vline(gfx_t *gfx, uint32_t color, int x, int y, int length) {
  int i, e;
  int w = gfx->w;
  int h = gfx->h;
  uint32_t *d = gfx->data;
  if ((uint32_t)x >= (uint32_t)w) return;
  if (y < 0) {
    length = y + length;
    y = 0;
  }
  if (y+length > h) length = h - y;
  i = y*w + x;
  e = i + length*w;
  for (; i < e; i+=w) {
    d[i] = color;
  }
}

void gfx_line(gfx_t *gfx, uint32_t color, int sx, int sy, int dx, int dy) {
  int t, x, y, xlen, ylen, incr, p = 0;
  if (sx == dx) {
    if (sy < dy) gfx_vline(gfx, color, sx, sy, dy - sy + 1);
    else gfx_vline(gfx, color, dx, dy, sy - dy + 1);
    return;
  }
  if (sy == dy) {
    if (sx < dx) gfx_hline(gfx, color, sx, sy, dx - sx + 1);
    else gfx_hline(gfx, color, dx, dy, sx - dx + 1);
    return;
  }

  gfx_set(gfx, dx, dy, color); // in some cases it skips last cell

  if (sy > dy) {
    t = sx;
    sx = dx;
    dx = t;
    t = sy;
    sy = dy;
    dy = t;
  }
  ylen = dy - sy;

  if (sx > dx) {
    xlen = sx - dx;
    incr = -1;
  } else {
    xlen = dx - sx;
    incr = 1;
  }

  y = sy;
  x = sx;

  if (xlen > ylen) {
    if (sx > dx) {
      t = sx;
      sx = dx;
      dx = t;
      y = dy;
    }

    p = (ylen << 1) - xlen;
    for (x = sx; x < dx; ++x) {
      gfx_set(gfx, x, y, color);
      if (p >= 0) {
        y += incr;
        p += (ylen - xlen) << 1;
      } else {
        p += (ylen << 1);
      }
    }
    return;
  }

  if (ylen > xlen) {
    p = (xlen << 1) - ylen;

    for (y = sy; y < dy; ++y) {
      gfx_set(gfx, x, y, color);
      if (p >= 0) {
        x += incr;
        p += (xlen - ylen) << 1;
      } else {
        p += (xlen << 1);
      }
    }
		
    return;
  }

  if (ylen == xlen) {
    while (y != dy) {
      gfx_set(gfx, x, y, color);
      x += incr;
      ++y;
    }
  }
}

void gfx_rect(gfx_t *gfx, uint32_t color, int fill, int x, int y, int w, int h) {
  if (fill) {
    int e = y+h;
    for (; y < e; y++) gfx_hline(gfx, color, x, y, w);
  } else {
    gfx_hline(gfx, color, x, y, w);
    gfx_hline(gfx, color, x, y+h - 1, w);
    gfx_vline(gfx, color, x, y+1, h-2);
    gfx_vline(gfx, color, x+w-1, y+1, h-2);
  }
}

static void gfx_circle_empty(gfx_t *gfx, uint32_t color, int x, int y, int r) {
  int p = 1 - r;
  int px = 0;
  int py = r;
  for (; px <= py + 1; px++) {
    gfx_set(gfx, x+px, y+py, color);
    gfx_set(gfx, x+px, y-py, color);
    gfx_set(gfx, x-px, y+py, color);
    gfx_set(gfx, x-px, y-py, color);
    gfx_set(gfx, x+py, y+px, color);
    gfx_set(gfx, x+py, y-px, color);
    gfx_set(gfx, x-py, y+px, color);
    gfx_set(gfx, x-py, y-px, color);
    if (p < 0) p += 2*px + 3;
    else {
      p += 2*(px - py) + 5;
      py--;
    }
  }
}

static void gfx_circle_filled(gfx_t *gfx, uint32_t color, int x, int y, int r) {
  int p = 1 - r;
  int px = 0;
  int py = r;
  for (; px <= py; px++) {
    gfx_vline(gfx, color, x+px, y,    py+1);
    gfx_vline(gfx, color, x+px, y-py, py);
    if (px != 0) {
      gfx_vline(gfx, color, x-px, y,    py+1);
      gfx_vline(gfx, color, x-px, y-py, py);
    }
    if (p < 0) p += 2*px + 3;
    else {
      p += 2*(px - py) + 5;
      py--;
      if (py >= px) {
        gfx_vline(gfx, color, x+py+1, y,    px+1);
        gfx_vline(gfx, color, x+py+1, y-px, px);
        gfx_vline(gfx, color, x-py-1, y,    px+1);
        gfx_vline(gfx, color, x-py-1, y-px, px);
      }
    }
  }
}

void gfx_circle(gfx_t *gfx, uint32_t color, int fill, int x, int y, int r) {
  if (fill) gfx_circle_filled(gfx, color, x, y, r);
  else gfx_circle_empty(gfx, color, x, y, r);
}

typedef struct lerp {
  double x;
  double i;
} lerp;

static void lerp_init(lerp *l, int sx, int ex, int first_step, int steps) {
  l->i = (double)(ex - sx) / steps;
  l->x = (double)sx + first_step*l->i;
}

static void lerp_advance(lerp *l) {
  l->x += l->i;
}

#define TRIANGLE_ROW(a,b) do { \
  int x1 = (int)a.x; \
  int x2 = (int)b.x; \
  if (x1 < x2) gfx_hline(gfx, color, x1, y, x2-x1); \
  else gfx_hline(gfx, color, x2, y, x1-x2); \
} while (0)

void gfx_triangle(gfx_t *gfx, uint32_t color, int ax, int ay, int bx, int by, int cx, int cy) {
  int beg_y, cen_y, end_y;
  int y, e;
  lerp l, r;

  if(ax < 0 && bx < 0 && cx < 0) return;
  if(ax >= gfx->w && bx >= gfx->w && cx >= gfx->w) return;
  if(ax == bx && ax == cx) return;

  if (ay > by) {
    SWAP(ax,bx);
    SWAP(ay,by);
  }

  if (ay > cy) {
    SWAP(ax,cx);
    SWAP(ay,cy);
  }

  if(by > cy) {
    SWAP(bx,cx);
    SWAP(by,cy);
  }

  beg_y = ay;
  cen_y = by;
  end_y = cy;

  if(end_y == beg_y || end_y < 0 || beg_y >= gfx->h) return;

  if (beg_y < 0) {
    lerp_init(&r, ax, cx, -beg_y, end_y-beg_y);
    y = 0;
  } else {
    lerp_init(&r, ax, cx, 0, end_y-beg_y);
    y = beg_y;
  }

  if (y < cen_y) {
    if (beg_y < 0) lerp_init(&l, ax, bx,-beg_y, cen_y-beg_y);
    else lerp_init(&l, ax, bx, 0, cen_y-beg_y);

    if (cen_y > gfx->h) e = gfx->h;
    else e = cen_y;

    for (; y < e; ++y) {
      TRIANGLE_ROW(l,r);
      lerp_advance(&l);
      lerp_advance(&r);
    }
  }

  if(cen_y < end_y) {
    lerp_init(&l, bx, cx, y-cen_y, end_y-cen_y);
    if (end_y > gfx->h) end_y = gfx->h;
    
    for (; y < end_y; ++y) {
      TRIANGLE_ROW(l,r);
      lerp_advance(&l);
      lerp_advance(&r);
    }
  }
}

static int show_error = 1;

void gfx_set_bflags_clear(gfx_t *gfx) {
  gfx->bflags = 0;
}

void gfx_set_bflags_flip_x(gfx_t *gfx) {
  gfx->bflags |= GFX_BFLAGS_FLIP_X;
}

void gfx_set_bflags_flip_y(gfx_t *gfx) {
  gfx->bflags |= GFX_BFLAGS_FLIP_Y;
}

void gfx_set_blit_alpha(gfx_t *gfx, int amount) {
  gfx->bflags |= GFX_BFLAGS_ALPHA;
  gfx->alpha = amount;
}

void gfx_set_blit_bright(gfx_t *gfx, int amount) {
  gfx->bflags |= GFX_BFLAGS_BRIGHTEN;
  gfx->blit_bright = amount;
}

void gfx_set_blit_light(gfx_t *gfx, uint32_t color) {
  gfx->bflags |= GFX_BFLAGS_LIGHT;
  gfx->color = color;
}

void gfx_set_blit_rect(gfx_t *gfx, int x, int y, int w, int h) {
  gfx->bx = x;
  gfx->by = y;
  gfx->bw = w;
  gfx->bh = h;
  gfx->bflags|=GFX_BFLAGS_RECT;
}

void gfx_set_recolor_map(gfx_t *gfx, uint32_t *map) {
  gfx->recolor_map = map;
}

void gfx_set_bumplight(gfx_t *gfx, int lx, int ly) {
  gfx->lx = lx;
  gfx->ly = ly;
  gfx->bflags|=GFX_BFLAGS_BUMPLIGHT;
}

#define begin_blit() \
  while (y < ey) { \
    bmvx = 0; \
    pd = y*dw + x; \
    ex = pd + w; \
    ps = sy*sw + sx; \
    while (pd < ex) { \
      do { \

#define end_blit(output) \
        DC = (output); \
      } while (0); \
      pd += 1; \
      ps += xi; \
    } \
    y += 1; \
    sy += yi; \
  } \

//source and destination colors
#define SC s[ps]
#define DC d[pd]

#include "blur.h"


static int near_alpha(gfx_t *gfx, int x, int y) {
  int i;
  uint32_t c;
  int xs[] = {-1, 1, 1, -1, 0, 0,-1,  1};
  int ys[] = {-1, 1, 0,  0, 1,-1, 1, -1};
  for (i=0; i<8; i++) {
    int dx = xs[i];
    int dy = ys[i];
    c = gfx_get(gfx,x+dx,y+dy);
    if (c&0xFF000000) return 1;
  }
  return 0;
}

static void gfx_gen_bumpmap(gfx_t *gfx) {
  int x,y,i,r,g,b,a,bmv;
  int w = gfx->w;
  int h = gfx->h;
  int n = w*h;
  uint32_t *s = gfx->data;
  uint32_t c;
  int *src = (int*)malloc(n*sizeof(int));
  int *dst = (int*)malloc(n*sizeof(int));

  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      i = y*w+x;
      c = s[i];
      fromR8G8B8(r,g,b,c);

      bmv = (r+g+b+127)/4;
      src[i] = bmv;
    }
  }

  gaussian_blur(dst, src, gfx->w, gfx->h, 1);
  gfx->bumpmap = (uint8_t*)malloc(n);
  for (i=0; i<n; i++) gfx->bumpmap[i] = dst[i];

  for (y = 1; y < h-1; y++) {
    for (x = 1; x < w-1; x++) {
      if (near_alpha(gfx,x,y)) {
        i = y*w+x;
        c = s[i];
        fromR8G8B8A8(r,g,b,a,c);

        bmv = (r+g+b+127)/4;
        gfx->bumpmap[i] = bmv;
      }
    }
  }
  free(src);
  free(dst);
}

#define BRIGHTEN(R,G,B) \
  do { \
    R = clamp_byte(R+bright); \
    G = clamp_byte(G+bright); \
    B = clamp_byte(B+bright); \
  } while(0)

#define LM_SIZE 256
static uint8_t lightmap[LM_SIZE*LM_SIZE];
static int lightmap_ready;
void init_lightmap() {
  int x, y;
  for (y = 0; y < LM_SIZE; y++) {
    for (x = 0; x < LM_SIZE; x++) {
      double dx = (double)x-128.0;
      double dy = (double)y-128.0;
      int v = 255-(int)(sqrt(dx*dx + dy*dy)*255.0/128.0);
      v=v*2/3;
      v = clamp_byte(v);
      lightmap[y*LM_SIZE+x] = (uint8_t)v;
    }
  }
  lightmap_ready = 1;
}

#define BLIT_BUMPLIGHT \
  if (light) { \
    int dx, dy, bmv, BMV, px, py, intensity; \
    bmv = bm[ps]; /*bumpmap value*/ \
    BMV = bmv + 128; \
    px = prev_bmv; \
    py = prev_bmv_line[bmvx]; \
    if (px < 0) px = bmv; \
    if (py < 0) py = bmv; \
    dx = BMV - px; \
    dy = BMV - py; \
    /*follow two lines are useful for dungeon lighting*/ \
    /*dx = dx + (bmvx%256)-lx;*/ \
    /*dy = dy + (y%256)-ly;*/ \
    dx = dx - lx; \
    dy = dy - ly; \
    /*dx = bmvx%256;*/ \
    /*dy = y%256;*/ \
    if (0 <= dx && dx < LM_SIZE && 0 <= dy && dy < LM_SIZE) { \
      intensity = lightmap[LM_SIZE*dy+dx]; \
    } else { \
      intensity = 0; \
    } \
    sr = sr*intensity/128; \
    sg = sg*intensity/128; \
    sb = sb*intensity/128; \
    sr = clamp_byte(sr); \
    sg = clamp_byte(sg); \
    sb = clamp_byte(sb); \
    /*sr = bmv; sg = bmv; sb = bmv;*/ \
    if (sa != 0xFF) { \
      prev_bmv_line[bmvx] = bmv; \
      prev_bmv = bmv; \
    } else { \
      prev_bmv_line[bmvx] = -1; \
      prev_bmv = -1; \
    } \
    ++bmvx; \
  }

/*
typedef struct LookupLevel LookupLevel;
struct LookupLevel {
  uint8_t Values[256];
};


typedef struct LookupTable LookupTable;

struct LookupTable {
  LookupLevel Levels[256];
};

static LookupTable AlphaTable;*/


static uint8_t AlphaTable[256][256];

static void ab_init() {
  float fValue, fAlpha;
  int iValue, iAlpha;
  for (iAlpha = 0; iAlpha < 256; iAlpha++) {
    fAlpha = ((float)iAlpha) / 255;
    for (iValue = 0; iValue < 256; iValue++) {
      fValue = ((float)iValue) / 255;
      AlphaTable[iAlpha][iValue] = clamp_byte((int)((fValue * fAlpha) * 255));
    }
  }
}

static int ready;

#define ALPHA_BLEND1(dr,dg,db,sr,sg,sb,Alpha) do { \
  uint8_t *st = AlphaTable[255-Alpha]; \
  uint8_t *dt = AlphaTable[Alpha]; \
  sr = st[sr]+dt[dr]; \
  sg = st[sg]+dt[dg]; \
  sb = st[sb]+dt[db]; \
} while(0)

#define ALPHA_BLEND1DA(dr,dg,db,da,sr,sg,sb,Alpha) do { \
  uint8_t *st = AlphaTable[255-clamp_byte(Alpha-da)]; \
  uint8_t *dt = AlphaTable[clamp_byte(Alpha+da)]; \
  sr = st[sr]+dt[dr]; \
  sg = st[sg]+dt[dg]; \
  sb = st[sb]+dt[db]; \
  sa = st[sa]+dt[da]; \
} while(0)

#define ALPHA_BLEND2(result, dst, src) do { \
  uint32_t alpha = 0xff-(src>>24); \
  alpha += (alpha > 0); \
  uint32_t srb = src & 0xff00ff; \
  uint32_t sg  = src & 0x00ff00; \
  uint32_t drb = dst & 0xff00ff; \
  uint32_t dg  = dst & 0x00ff00; \
  uint32_t orb = (drb + (((srb - drb) * alpha + 0x800080) >> 8)) & 0xff00ff; \
  uint32_t og  = (dg  + (((sg  - dg ) * alpha + 0x008000) >> 8)) & 0x00ff00; \
  result = orb+og + (src&0xFF000000); \
} while(0)


//NOTE: X>>8 is a division by 256, while max alpha is 0xFF
//      this leads to some loss of precision
#define ALPHA_BLEND3 \
    sm = 0xFF - sa; \
    sr = (sr*sm + dr*sa)>>8; \
    sg = (sg*sm + dg*sa)>>8; \
    sb = (sb*sm + db*sa)>>8; \
    c = R8G8B8(sr,sg,sb);

#define ALPHA_BLEND \
  if (DC&0xFF000000) { \
    if (bright|light) { c = R8G8B8(sr,sg,sb); } \
    else  { \
      int dr, dg, db, da;\
      fromR8G8B8(dr,dg,db,DC);\
      ALPHA_BLEND1DA(dr,dg,db,da,sr,sg,sb,sa); \
      c = R8G8B8A8(sr,sg,sb,sa); \
    } \
  } else {\
    int dr, dg, db, da;\
    fromR8G8B8(dr,dg,db,DC);\
    ALPHA_BLEND1(dr,dg,db,sr,sg,sb,sa); \
    c = R8G8B8(sr,sg,sb); \
  }

//light energy divisor
//#define EDIV (0x100*4*32)
#define EDIV 15

INLINE uint8_t lighten(int t, int e) {
  return clamp_byte(t + ((t*e)>>EDIV));
}

INLINE uint8_t darken(int t, int e) {
  int tt = ((t*e)>>EDIV);
  if (tt>t) tt = t;
  return t - tt;
}

void gfx_blit(gfx_t *gfx, int x, int y, gfx_t *src) {
  int i, r, g, b, a;
  int sr, sg, sb, sa;
  uint32_t c;
  gfx_t *dst = gfx;
  int cx = 0;
  int cy = 0;
  int cw = dst->w;
  int ch = dst->h;
  int ow;
  int oh;
  int xi; // x increment
  int yi; // y increment
  int ex = 0;
  int ey = 0;
  uint32_t *d = dst->data;
  int dw = dst->w;
  uint32_t *s = src->data;
  int sw = src->w;
  int sh = src->h;
  uint32_t *m = src->recolor_map ? src->recolor_map : src->cmap;
  int pd = 0; // destination pointer
  int ps = 0; // sorce pointer
  int flip_x = src->bflags&GFX_BFLAGS_FLIP_X;
  int flip_y = src->bflags&GFX_BFLAGS_FLIP_Y;
  int light = src->bflags&GFX_BFLAGS_LIGHT;
  int bright = 0;
  uint32_t alpha = 0;
  int sx, sy, w, h; //source rect
  int bumplight = 0;
  int lx,ly;
  int bmvx;
  int prev_bmv; //previous bumpmap value
  int prev_bmv_line[2048];
  int special;
  uint8_t *bm = src->bumpmap;

  if (!src->opacity) {
    determine_opacity(src);
  }

  if (!ready) {
    ab_init();
    ready = 1;
  }

  if (src->bflags & GFX_BFLAGS_RECT) {
    sx = src->bx;
    sy = src->by;
    w = src->bw;
    h = src->bh;
  } else {
    sx = 0;
    sy = 0;
    w = src->w;
    h = src->h;
  }

  if (src->bflags & GFX_BFLAGS_BRIGHTEN) {
    bright = src->blit_bright;
  }

  if (src->bflags & GFX_BFLAGS_ALPHA) {
    alpha = src->alpha;
  }

  if (src->bflags & GFX_BFLAGS_BUMPLIGHT) {
    if (!lightmap_ready) init_lightmap();
    if (!src->bumpmap) gfx_gen_bumpmap(src);
    bumplight = 1;
    bm = src->bumpmap;
    lx = src->lx;
    ly = src->ly;
    prev_bmv = -1;
    for (i=0; i < 2048; i++) prev_bmv_line[i] = -1;
  }

  src->bflags = 0;
  src->recolor_map = 0;

  x += flip_x ? -src->x : src->x;
  y += flip_y ? -src->y : src->y;

  if (sx < 0) {
    w += sx;
    sx = 0;
  }

  if (sy < 0) {
    h += sy;
    sy = 0;
  }

  if (sx + w >= sw) w = sw - sx;
  if (sy + h >= sh) h = sh - sy;

  if (x >= cw || y >= ch) return;
  if (x+w <= cx || y+h <= cy) return;

  ow = w;
  oh = h;

  if (x < cx) {
    int i = cx - x;
    if (flip_x) ow -= i;
    else sx += i;
    w -= i;
    x = cx;
  }

  if (y < cy) {
    int i = cy - y;
    if (flip_y) oh -= i;
    else sy += i;
    h -= i;
    y = cy;
  }

  ey = y + h;

  if (x+w > cw) w = cw - x;
  if (ey > ch) ey = ch;

  if (flip_x) {
    sx = sx + ow - 1;
    xi = -1;
  } else {
    xi = 1;
  }

  if (flip_y) {
    sy = sy + oh - 1;
    yi = -1;
  } else {
    yi = 1;
  }

  //fprintf(stderr, "%dx%d: %d,%d:%dx%d %d\n", sw,sh, sx, sy, w,h, ey-y);

  special = bright|bumplight|alpha;
  if (dst->cmap) {
    if (!src->cmap) {
      fprintf(stderr, "gfx.c: can't blit truecolor into indexed\n");
      abort();
    }
    begin_blit()
    c = SC;
    end_blit(c)
  } else if(!special && m) {

    begin_blit()
    c = m[SC];
    if (c&0xFF000000) c = DC;
    end_blit(c)

  } else if (special && m) {
    begin_blit()
    c = m[SC];

    if ((c&0xFF000000)==0xFF000000) {
      c = DC;
      goto blit_end3;
    }

    fromR8G8B8A8(sr,sg,sb,sa,c);

    if (special) {
      if (alpha) {
        sa += alpha;
        if (sa >= 0xff) {
          c = DC;
          goto blit_end3;
        }
      }
      if (bright) { BRIGHTEN(sr,sg,sb); }
      //BLIT_BUMPLIGHT
      if (sa == 0) {

        if (bright|bumplight) { c = R8G8B8(sr,sg,sb); }
        goto blit_end3;
      }
    }

    ALPHA_BLEND

blit_end3:;
    end_blit(c)
  } else if (light) {
    fromR8G8B8(sr,sg,sb,src->color);
    begin_blit()
    int alpha = SC&0xFF;
    fromR8G8B8(r,g,b,DC);
    c = R8G8B8(lighten(r,alpha*sr),lighten(g,alpha*sg),lighten(b,alpha*sb));
    end_blit(c);
  } else if(!special && src->opacity != OCT_ALPHA) {

    begin_blit()
    c = SC;
    sa = c>>31;
    c = c + (DC - c)*sa;
    end_blit(c)
  } else if(!special) {

    begin_blit()
    int sm; // source multiplier
    c = SC;

    if ((c&0xFF000000)==0xFF000000) {
      c = DC;
      goto blit_end;
    }

    fromR8G8B8A8(sr,sg,sb,sa,c);

    ALPHA_BLEND
blit_end:;
    end_blit(c)
  } else {

    begin_blit()
    int sm; // source multiplier

    c = SC;

    if ((c&0xFF000000)==0xFF000000) {
      c = DC;
      goto blit_end2;
    }

    fromR8G8B8A8(sr,sg,sb,sa,c);

    if (special) {
      if (alpha) {
        sa += alpha;
        if (sa >= 0xff) {
          c = DC;
          goto blit_end2;
        }
      }
      if (bright) { BRIGHTEN(sr,sg,sb); }
      //BLIT_BUMPLIGHT
      if (sa == 0) {

        if (bright|bumplight) { c = R8G8B8(sr,sg,sb); }
        goto blit_end2;
      }
    }
    ALPHA_BLEND
blit_end2:;
    end_blit(c);
  }
}

#undef SC
#undef DC

static uint32_t margins_result[4];

//determines sprite boundaries inside alpha=255 empty area;
//used for packing sprites.
void *gfx_margins(gfx_t *gfx) {
  int w = gfx->w;
  int h = gfx->h;
  uint32_t *d = gfx->data;
  uint32_t *m = gfx->cmap;
  int x1 = w;
  int x2 = -1;
  int sx = 0;
  int xb = w;
  int xe = 0;
  int yb = h;
  int ye = 0;
  int x;
  int y;
  for (y = 0; y < h; y++) {
    sx = y*w;
    xb = w;
    xe = -1;
    for (x = 0; x < w; x++) {
      uint32_t c = d[x+sx];
      if (m) c = m[c];
      if ((c>>24) != 255) {
        if (xb == w) xb = x;
        xe = x;
      }
    }
    if (xe != -1) {
      if (yb == h) yb = y;
      if (xb < x1) x1 = xb;
      if (xe > x2) x2 = xe;
      ye = y;
    }
  }
  if (x1 != w) {
    margins_result[0] = x1;
    margins_result[1] = yb;
    margins_result[2] = x2-x1+1;
    margins_result[3] = ye-yb+1;
  } else {
    margins_result[0] = 0;
    margins_result[1] = 0;
    margins_result[2] = w;
    margins_result[3] = h;
  }
  return margins_result;
}


#if 1
uint32_t array[] = {123,456,789};

void *test(char *name, int x, float y) {
  fprintf(stderr, "%d:%f: Hello, %s! %d,%d,%d\n", x, y, name, array[0], array[1], array[2]);
  return (void*)array;
}

static uint8_t sqrt_50_256[] = {
  114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,
  133,134,135,136,137,138,139,140,141,142,143,143,144,145,146,147,148,149,150,
  150,151,152,153,154,155,155,156,157,158,159,159,160,161,162,163,163,164,165,
  166,167,167,168,169,170,170,171,172,173,173,174,175,175,176,177,178,178,179,
  180,181,181,182,183,183,184,185,185,186,187,187,188,189,189,190,191,191,192,
  193,193,194,195,195,196,197,197,198,199,199,200,201,201,202,203,203,204,204,
  205,206,206,207,207,208,209,209,210,211,211,212,212,213,214,214,215,215,216,
  217,217,218,218,219,219,220,221,221,222,222,223,223,224,225,225,226,226,227,
  227,228,229,229,230,230,231,231,232,232,233,234,234,235,235,236,236,237,237,
  238,238,239,239,240,241,241,242,242,243,243,244,244,245,245,246,246,247,247,
  248,248,249,249,250,250,251,251,252,252,253,253,254,254,255,255
};

static uint8_t sqrt_0_10[] = {
  1,2,3,3,4,4,5,5,5,6,6,6,7,7,7,7,8,8,8,8,9,9,9,9,9,10,10,10,10,10,11,11,11,
  11,11,11,12,12,12,12,12,12,13,13,13,13,13,13,13,14,14,14,14,14,14,14,15,15,
  15,15,15,15,15,15,16,16,16,16,16,16,16,16,17,17,17,17,17,17,17,17,17,18,18,
  18,18,18,18,18,18,18,19,19,19,19,19,19,19,19,19,19,20,20,20,20,20,20,20,20,
  20,20,21,21,21,21,21,21,21,21,21,21,21,22,22,22,22,22,22,22,22,22,22,22,23,
  23,23,23,23,23,23,23,23,23,23,23,24,24,24,24,24,24,24,24,24,24,24,24,25,25,
  25,25,25,25,25,25,25,25,25,25,25,26,26,26,26,26,26,26,26,26,26,26,26,26,27,
  27,27,27,27,27,27,27,27,27,27,27,27,27,28,28,28,28,28,28,28,28,28,28,28,28,
  28,28,29,29,29,29,29,29,29,29,29,29,29,29,29,29,29,30,30,30,30,30,30,30,30,
  30,30,30,30,30,30,30,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,31,32,32,
  32,32,32,32,32,32,32,32,32,32,32,32,32,32,33,33,33,33,33,33,33,33,33,33,33,
  33,33,33,33,33,33,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,34,35,35,
  35,35,35,35,35,35,35,35,35,35,35,35,35,35,35,35,36,36,36,36,36,36,36,36,36,
  36,36,36,36,36,36,36,36,36,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,37,
  37,37,37,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,38,39,39,39,
  39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,40,40,40,40,40,40,40,40,
  40,40,40,40,40,40,40,40,40,40,40,40,41,41,41,41,41,41,41,41,41,41,41,41,41,
  41,41,41,41,41,41,41,41,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,42,
  42,42,42,42,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,43,
  43,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,45,45,
  45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,45,46,46,46,46,
  46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,46,47,47,47,47,47,47,
  47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,48,48,48,48,48,48,48,
  48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,48,49,49,49,49,49,49,49,49,
  49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,49,50,50,50,50,50,50,50,50,
  50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,51,51,51,51,51,51,51,51,
  51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,52,52,52,52,52,52,52,
  52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,53,53
};

static uint8_t sqrt_11_49[] = {
  54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,0,76,77,78,
  0,79,80,81,82,83,0,84,85,86,0,87,88,89,0,90,0,91,92,93,0,94,0,95,96,97,0,98,0,
  99,100,101,0,102,0,103,0,104,105,106,0,107,0,108,0,109,0,110,0,111,112,113
};

uint16_t isqrt16(uint16_t v) {
  uint16_t a, b;
  uint16_t h = v>>8;
  if (h <= 10) return v ? sqrt_0_10[v>>2] : 0;
  if (h >= 50) return sqrt_50_256[h-50];
  h = (h-11)<<1;
  a = sqrt_11_49[h];
  b = sqrt_11_49[h+1];
  if (!a) return b;
  return b*b > v ? a : b;
}

int main(int argc, char **argv) {
  int x,y;
  gfx_t *gfx;
  
  int i;
  for (i = 0; i < 256*256; i++) {
    float fs = sqrtf((float)i);
    int is = isqrt16(i);
    printf("%d,%f\n", is, fs);
  }
  
  exit(-1);

  if (argc != 3) {
    printf("Usage: %s <infile> <outfile>\n", argv[0]);
    return -1;
  }
  gfx = gfx_load_png(argv[1]);
  if (!gfx) return -1;
  gfx_to_luv(gfx);
  for (y = 0; y < gfx->h; y++) for (x = 0; x < gfx->w; x++) {
    uint32_t luv = gfx_get(gfx, x, y);
    //luv = luv_rotate(0.5, luv);
    //luv = luv_saturate(0.5, luv);
    gfx_set(gfx, x,y, luv);
  }
  gfx_to_rgb(gfx);
  gfx_save_png(argv[2], gfx);

  /*gfx = new_gfx(256, 256);
  for (y = 0; y < 256; y++) for (x = 0; x < 256; x++) {
    uint32_t rgb = luv2rgb((200<<16) + (y<<8) + x);
    uint32_t luv = rgb2luv(rgb);
    rgb = luv2rgb(luv);
    gfx_set(gfx, x,y, rgb);
  }
  gfx_save_png("./uv_spectrum.png", gfx);*/

  fprintf(stderr, "%dx%d\n", gfx->w, gfx->h);

  return 0;
}
#endif