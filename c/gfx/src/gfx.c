#include <math.h>
#include "gfx.h"
#include "common.h"
#include "cfp.h"

// The "multithreaded blitter" used to ping-pong two `blit_t` buffers
// and hand work to a worker thread, but every public entry point took
// a busy-wait `while(blit_in_progress);` first, so the queue depth was
// 1 and the worker thread added overhead without ever overlapping
// work. We collapsed it to a single buffer and a direct call. See
// ui_speedup.md (C1). The `blt`/`pbl` names are kept since gfx_blit.c
// and the public setters (gfx_set_blit_fade, gfx_set_blit_bright, ...)
// read/write through them.
//
// GMUTEX used to busy-spin on the worker; it's now a no-op. The
// remaining uses are kept as a marker of "this entry point used to be
// the sync point", in case we ever re-introduce real parallelism here.
// The original macro ended in a semicolon and most call sites rely on
// that — they don't write a `;` after GMUTEX. Preserved here.
#define GMUTEX do {} while (0);

static blit_t blits[1];
blit_t *blt = blits;
// `pbl` is also declared `extern ... *volatile pbl` in common.h, where
// it used to be the worker-thread's read pointer. Keep the volatile
// qualifier so the declaration and definition agree.
blit_t *volatile pbl = blits;

//default gamma lookup table (2.2 for sRGB)
extern gfx_gamma_t dgam;

static void init_blit_thread(void) {
  blits[0].prev_br = 0x10000;
}

void gfx_sync(gfx_t *gfx) {
  (void)gfx; //no-op: there's no async work to flush anymore
}

void gfx_blit(gfx_t *gfx, int x, int y, gfx_t *src) {
  _gfx_blit(gfx, x, y, src);
}


void _gfx_init_tables();

static int ready;

static void init_gfx(gfx_t *gfx, uint32_t w, uint32_t h) {
  gfx->w = w;
  gfx->h = h;
  gfx->x = 0;
  gfx->y = 0;
  gfx->flags = 0;
  gfx->rmap = 0;
  gfx->scissor = 0;
  gfx->gamma = &dgam;
}

gfx_t *new_gfx(uint32_t w, uint32_t h) {
  gfx_t *gfx = (gfx_t*)malloc(sizeof(gfx_t)+w*h*sizeof(uint32_t));
  if (!gfx) return 0;

  if (!ready) {
    _gfx_init_tables();
    init_blit_thread();
    ready = 1;
  }

  init_gfx(gfx, w, h);

  return gfx;
}

static gfx_t *new_rle_gfx(gfx_t *src, uint16_t **rle, uint32_t nopaq) {
  int w = src->w, h = src->h;
  gfx_t *gfx = (gfx_t*)malloc(sizeof(gfx_t)+nopaq*sizeof(uint32_t));
  if (!gfx) return 0;

  gfx->rle = rle;

  init_gfx(gfx, w, h);
  gfx->x = src->x;
  gfx->y = src->y;

  if (!src->rmap) gfx->rmap = 0;
  else {
    int rsz = src->nrecolors*sizeof(uint32_t)*2;
    gfx->rmap = malloc(nopaq*sizeof(uint8_t));
    gfx->recolors = (uint32_t*)malloc(rsz);
    gfx->nrecolors = src->nrecolors;
    memcpy(gfx->recolors, src->recolors, rsz);
  }

  gfx->flags = src->flags|GFX_F_RLE;

  return gfx;
}

static void gfx_free_rmap(gfx_t *gfx) {
  free(gfx->rmap);
  free(gfx->recolors);
  gfx->rmap = 0;
}

static void gfx_alloc_rmap(gfx_t *gfx, int nrecolors) {
  gfx->rmap = (uint8_t*)malloc(gfx->w*gfx->h);
  gfx->recolors = (uint32_t*)malloc(nrecolors*sizeof(uint32_t)*2);
  gfx->nrecolors = nrecolors;
}

void gfx_strip_rle(gfx_t *gfx) {
  int y;
  if (!(gfx->flags&GFX_F_RLE)) return;
  GMUTEX
  for (y = 0; y < gfx->h; y++) free(gfx->rle[y]);
  free(gfx->rle);
  gfx->flags ^= GFX_F_RLE;
}

void _free_gfx(gfx_t *gfx) {
  gfx_strip_rle(gfx);
  if (gfx->rmap) gfx_free_rmap(gfx);
  if (gfx->gamma != &dgam) free(gfx->gamma);
  if (gfx->scissor) {
    scissor_t *s = gfx->scissor;
    gfx->scissor = s->next;
    free(s);
  }
  free(gfx);
}


void free_gfx(gfx_t *gfx) {
  GMUTEX
  _free_gfx(gfx);
}

void gfx_set_gamma(gfx_t *gfx, float value) {
  GMUTEX
  if (value == 1.0f) {
    //just use direct gamma space blend
    gfx->flags |= GFX_BF_GBLEND;
    return;
  }
  gfx->flags &= ~GFX_BF_GBLEND;
  if (gfx->gamma == &dgam) {
    gfx->gamma = malloc(sizeof(gfx_gamma_t));
    gfx_gamma_init(gfx->gamma, value);
    return;
  }
  if (gfx->gamma->value == value) return;
  gfx_gamma_init(gfx->gamma, value);
}

uint32_t gfx_w(gfx_t *gfx) {
  return gfx->w;
}

uint32_t gfx_h(gfx_t *gfx) {
  return gfx->h;
}

uint32_t *gfx_pixels(gfx_t *gfx) {
  GMUTEX
  return gfx->data;
}

uint8_t *gfx_mask(gfx_t *gfx) {
  return 0;
}

int gfx_x(gfx_t *gfx) {
  return gfx->x;
}

int gfx_y(gfx_t *gfx) {
  return gfx->y;
}

void gfx_set_xy(gfx_t *gfx, int x, int y) {
  GMUTEX
  gfx->x = x;
  gfx->y = y;
}

int dbg = 0;

uint32_t gfx_get(gfx_t *gfx, int x, int y) {
  if (UNLIKELY((uint32_t)x >= (uint32_t)gfx->w)) return 0;
  if (UNLIKELY((uint32_t)y >= (uint32_t)gfx->h)) return 0;
  return gfx->data[gfx->w*y+x];
}

void gfx_set(gfx_t *gfx, int x, int y, uint32_t color) {
  if (UNLIKELY((uint32_t)x >= (uint32_t)gfx->w)) return;
  if (UNLIKELY((uint32_t)y >= (uint32_t)gfx->h)) return;
  gfx->data[gfx->w*y+x] = color;
}

//rle safe get
uint32_t gfx_rget(gfx_t *gfx, int x, int y) {
  int r = 0;
  if (!(gfx->flags & GFX_F_RLE)) return gfx_get(gfx,x,y);
  if (UNLIKELY((uint32_t)x >= (uint32_t)gfx->w)) return 0;
  if (UNLIKELY((uint32_t)y >= (uint32_t)gfx->h)) return 0;
  int rx = 0;
  int run;
  uint16_t *p = gfx->rle[y];
  uint32_t *s = gfx->data + *(uint32_t*)p;
  for (p += 2; ; p++) {
    if (rx + *p > x) return 0xFF000000; //completely transapred
    rx += *p;
    p++;
    if (rx + *p > x) {
      s += x-rx;
      return *s;
    }
    rx += *p;
    s += *p;
  } 
}

int gfx_dbg(gfx_t *gfx, int new_dbg_state) {
  int prev = dbg;
  dbg = new_dbg_state;
  return prev;
}


#include "memset4.h"

void _gfx_clear(gfx_t *gfx, uint32_t color) {
  memset4(gfx->data, color, gfx->w*gfx->h);
  gfx->flags &= ~(uint32_t)GFX_F_OCT;
}

void gfx_clear(gfx_t *gfx, uint32_t color) {
  _gfx_clear(gfx, color);
}

//nearest neighbour scale
void gfx_scaled(gfx_t *gfx, gfx_t *src) {
  GMUTEX
  uint32_t *p = gfx->data;
  uint32_t *sdata = src->data;
  uint32_t w = gfx->w;
  uint32_t h = gfx->h;
  uint32_t sw = src->w;
  uint32_t sh = src->h;
  uint32_t xr = (sw<<16)/w + 1;
  uint32_t yr = (sh<<16)/h + 1;
  uint32_t y;
  for (uint32_t y = 0; y < h; y++) {
    uint32_t *ps = sdata + ((y*yr)>>16)*sw;
    uint32_t r = 0;
    uint32_t *ex = p + (w&~(uint32_t)7);
#define SCALED_COPY *p++ = ps[r>>16]; r += xr;
    while (p < ex) {
      SCALED_COPY;
      SCALED_COPY;
      SCALED_COPY;
      SCALED_COPY;
      SCALED_COPY;
      SCALED_COPY;
      SCALED_COPY;
      SCALED_COPY;
    }
    ex += w&7;
    while (p < ex) {
      SCALED_COPY;
   }
  }
  gfx->flags = (gfx->flags&~(uint32_t)GFX_F_OCT)|(src->flags&GFX_F_OCT);
}

//Compared to Lanczos-2, Catmull-Rom has a partition of unity, which induces
//less ripple in solid colours where local clamping can't be easily done.

#define STBIR_DEFAULT_FILTER_UPSAMPLE     STBIR_FILTER_CATMULLROM
#define STBIR_DEFAULT_FILTER_DOWNSAMPLE   STBIR_FILTER_MITCHELL

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#if 0
int stbir_resize_uint8(     const unsigned char *input_pixels , int input_w , int input_h , int input_stride_in_bytes,
                                           unsigned char *output_pixels, int output_w, int output_h, int output_stride_in_bytes,
                                     int num_channels);
#endif

void gfx_scaled_smooth(gfx_t *gfx, gfx_t *src) {
  GMUTEX
  uint32_t *p = gfx->data;
  uint32_t *sdata = src->data;
  uint32_t w = gfx->w;
  uint32_t h = gfx->h;
  uint32_t sw = src->w;
  uint32_t sh = src->h;
#if 1
  stbir_resize_uint8((uint8_t*)src->data, src->w, src->h, 0,
                     (uint8_t*)gfx->data, gfx->w, gfx->h, 0,
                     4);
#else
  stbir_resize_uint8_srgb((uint8_t*)src->data, src->w, src->h, 0,
                          (uint8_t*)gfx->data, gfx->w, gfx->h, 0,
                          4, 3, STBIR_FLAG_ALPHA_PREMULTIPLIED);
#endif
  gfx->flags = (gfx->flags&~(uint32_t)GFX_F_OCT)|(src->flags&GFX_F_OCT);
}


// Function to sort an array in ascending order
static void sortArray(uint8_t *array, int size) {
  int i, j;
  uint8_t temp;

  for (i = 0; i < size - 1; i++) {
    for (j = i + 1; j < size; j++) {
      if (array[i] > array[j]) {
          temp = array[i];
          array[i] = array[j];
          array[j] = temp;
      }
    }
  }
}

// Function to apply median blur to an image
static void median_blur_channel(gfx_t *dst_gfx, gfx_t *src_gfx, int kernel_size, int channel) {
  GMUTEX
  int nchannels = 4;
 
  uint8_t *dst = (uint8_t *)dst_gfx->data + channel;
  int dst_w = dst_gfx->w;
  int dst_h = dst_gfx->h;
  uint8_t *src = (uint8_t *)src_gfx->data + channel;
  int src_w = src_gfx->w;
  int src_h = src_gfx->h;
  
  
  int kernel_radius = kernel_size / 2;
  int i, j, k, l;
  uint8_t *window = malloc(kernel_size*kernel_size + kernel_size);

  for (i = 0; i < dst_h; i++) {
    for (j = 0; j < dst_w; j++) {
      // Compute the window boundaries
      int x_start = j - kernel_radius;
      int y_start = i - kernel_radius;
      int x_end = j + kernel_radius;
      int y_end = i + kernel_radius;

      int window_index = 0;

      // Collect the pixel values within the window
      for (k = y_start; k <= y_end; k++) {
        int clamped_y = k < 0 ? 0 : (k >= src_h ? src_h - 1 : k);
        for (l = x_start; l <= x_end; l++) {
          // Clamp the window coordinates to image boundaries
          int clamped_x = l < 0 ? 0 : (l >= src_w ? src_w - 1 : l);
         

          // Get the pixel value at (clamped_x, clamped_y)
          window[window_index++] = src[(clamped_y * src_w + clamped_x)*nchannels];
        }
      }

      // Sort the window pixel values
      sortArray(window, kernel_size*kernel_size);

      // Set the median pixel value to the destination image
      *dst = window[(kernel_size * kernel_size) / 2];
      dst += nchannels;
    }
  }
  free(window);
}

void gfx_median_blur(gfx_t *gfx, gfx_t *src, int kernel_size) {
  for (int i = 0; i < 4; i++) {
    median_blur_channel(gfx, src, kernel_size, i);
  }
}


//nearest neighbour scale
void gfx_skewed(gfx_t *gfx, gfx_t *src, int amount) {
  GMUTEX
  uint32_t *p = gfx->data;
  uint32_t *sp = src->data;
  uint32_t sw = src->w;
  uint32_t w = gfx->w;
  uint32_t h = gfx->h;
  for (uint32_t y = 0; y < h; y++) {
    uint32_t sx = amount*y/(h-1);
    for (uint32_t x = 0; x < w; x++) {
      uint32_t c = 0xFF000000;
      uint32_t xx = sx - amount;
      if (xx < sw) c = sp[sw*y + xx];
      sx++;
      *p++ = c;
    }
  }
  gfx->flags = (gfx->flags&~(uint32_t)GFX_F_OCT)|(src->flags&GFX_F_OCT);
}



void gfx_determine_opacity(gfx_t *gfx) {
  uint32_t *d =  gfx->data;
  uint32_t *end = d+gfx->w*gfx->h;
  uint32_t opacity = GFX_OCT_OPAQUE;
  for (; d < end; d++) {
    uint8_t alpha = *d>>24;
    if (alpha) {
      if (alpha != 0xFF) {
        gfx->flags |= GFX_OCT_BLEND;
        return;
      }
      opacity = GFX_OCT_BINARY;
    }
  }
  gfx->flags |= opacity;
}

#include "gfx_primitives.h"

void gfx_set_bflags_clear(gfx_t *gfx) {
  blt->bflags = 0;
}

void gfx_set_bflags_flip_x(gfx_t *gfx) {
  blt->bflags |= GFX_BF_FLIP_X;
}

void gfx_set_bflags_flip_y(gfx_t *gfx) {
  blt->bflags |= GFX_BF_FLIP_Y;
}

static uint8_t br_linear[256] = {
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,
  26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
  49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,
  72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,
  95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,
  114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,
  132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,
  150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,
  168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,
  186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,
  204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,
  222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
  240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255
};

INLINE void fill_br_lut(uint8_t *lut, int br) {
  int i;
  for (i = 0; i < 256; i++, br++) {
    lut[i] = clamp_byte(br);
  }
}

INLINE void fill_br_lut_s(uint8_t *lut, float br, float s) {
  int i;
  for (i = 0; i < 256; i++, br += 1.0f) {
    lut[i] = clamp_byte((int)(br*s));
  }
}

INLINE void set_br_lut(uint8_t *lut, int val) {
  int i;
  for (i = 0; i < 256; i++) lut[i] = val;
}

void gfx_set_blit_fade(gfx_t *gfx, int amount) {
  if (!amount) return;
  if (!(blt->bflags&GFX_BF_BRIGHTEN)) {
    blt->bflags |= GFX_BF_BRIGHTEN;
    blt->r_br = br_linear;
    blt->g_br = br_linear;
    blt->b_br = br_linear;
  }
  if (blt->bflags&GFX_BF_ADDITIVE) {
  } else {
    blt->bflags |= GFX_BF_BLEND;
  }
  if (amount != blt->prev_fade) {
    blt->prev_fade = amount;
    fill_br_lut(blt->a_br_lut, amount);
  }
  blt->a_br = blt->a_br_lut;
}

void gfx_set_blit_bright(gfx_t *gfx, int br) {
  if (!(blt->bflags&GFX_BF_BRIGHTEN)) {
    blt->bflags |= GFX_BF_BRIGHTEN;
    blt->a_br = br_linear;
  }
  if (br != blt->prev_br) {
    blt->prev_br = br;
    fill_br_lut(blt->br_lut, br);
  }
  blt->r_br = blt->br_lut;
  blt->g_br = blt->br_lut;
  blt->b_br = blt->br_lut;
}

void gfx_set_blit_colorize(gfx_t *gfx, int br, float r, float g, float b) {
  blt->bflags |= GFX_BF_BRIGHTEN;
  if ( br != blt->prev_br  || r != blt->prev_c_r
    ||  g != blt->prev_c_g || b != blt->prev_c_b) {
    blt->prev_br = br;
    if (r != blt->prev_c_r) {
      blt->prev_c_r = r;
      fill_br_lut_s(blt->r_br_lut, br, r);
    }
    if (g != blt->prev_c_g) {
      blt->prev_c_g = g;
      fill_br_lut_s(blt->g_br_lut, br, g);
    }
    if (b != blt->prev_c_b) {
      blt->prev_c_b = b;
      fill_br_lut_s(blt->b_br_lut, br, b);
    }
  }
  blt->r_br = blt->r_br_lut;
  blt->g_br = blt->g_br_lut;
  blt->b_br = blt->b_br_lut;
  blt->a_br = br_linear;
}

void gfx_set_blit_eraser(gfx_t *gfx, uint32_t color) {
  blt->bflags |= GFX_BF_ERASER;
}

void gfx_set_blit_stencil(gfx_t *gfx, uint32_t color) {
  blt->bflags |= GFX_BF_BRIGHTEN;
  if (color != blt->prev_stencil) {
    int r,g,b,a;
    UNRGBA(r,g,b,a,color);
    blt->prev_stencil = color;
    set_br_lut(blt->r_st_lut, r);
    set_br_lut(blt->g_st_lut, g);
    set_br_lut(blt->b_st_lut, b);
    if (a) {
      fill_br_lut(blt->a_st_lut, a);
    }
  }
  blt->r_br = blt->r_st_lut;
  blt->g_br = blt->g_st_lut;
  blt->b_br = blt->b_st_lut;
  blt->a_br = (color&0xFF000000) ? blt->a_st_lut : br_linear;
}

//multiplicative blending (aka lightmapping)
void gfx_set_blit_light(gfx_t *gfx, float energy, uint32_t color) {
  blt->bflags |= GFX_BF_LIGHT;
  blt->light_energy = energy;
  blt->light_color = color;
}

//used to blen special effects, having halo around them
void gfx_set_blit_additive(gfx_t *gfx, float energy, uint32_t color) {
  blt->bflags |= GFX_BF_ADDITIVE;
  blt->light_energy = energy;
  blt->light_color = color;
}

void gfx_set_blit_rect(gfx_t *gfx, int x, int y, int w, int h) {
  blt->bx = x;
  blt->by = y;
  blt->bw = w;
  blt->bh = h;
  blt->bflags |= GFX_BF_RECT;
}

void gfx_scissor(gfx_t *gfx, int x, int y, int w, int h) {
  //ensure operation is finished
  GMUTEX
  scissor_t *p = gfx->scissor;
  if (p) {
    int px = p->rect[0];
    int py = p->rect[1];
    int pex = px + p->rect[2];
    int pey = py + p->rect[3];
    int ex = x+w;
    int ey = y+h;
    x = MIN(pex,MAX(px,x));
    y = MIN(pey,MAX(py,y));
    ex = MAX(px,MIN(pex,ex));
    ey = MAX(py,MIN(pey,ey));
    w = ex-x;
    h = ey-y;
  }
  scissor_t *s = malloc(sizeof(scissor_t));
  s->next = p;
  gfx->scissor = s;
  s->rect[0] = x;
  s->rect[1] = y;
  s->rect[2] = w;
  s->rect[3] = h;
}

void gfx_pop_scissor(gfx_t *gfx) {
  GMUTEX
  scissor_t *s = gfx->scissor;
  if (s) {
    gfx->scissor = s->next;
    free(s);
  }
}


void gfx_filter(gfx_t *gfx, gfx_t *dst, float *kernel, int size) {
  int x,y,dx,dy,xx,yy;
  int w = gfx->w;
  int h = gfx->h;
  int d = round(sqrt(size))/2;

  GMUTEX

  for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
    float r=0.0f, g=0.0f, b=0.0f, a=0.0f;
    float *k = kernel;
    for (dy = -d; dy <= d; dy++) {
      yy = y + dy;
      if ((uint32_t)yy >= (uint32_t)h) yy = y;
      yy *= w;
      for (dx = -d; dx <= d; dx++, k++) {
        xx = x + dx;
        if ((uint32_t)xx >= (uint32_t)w) xx = x;
        uint32_t c = gfx->data[yy + xx];
        int ir,ig,ib,ia;
        UNRGBA(ir,ig,ib,ia,c);
        float f = *k;
        r += (float)ir*f;
        g += (float)ig*f;
        b += (float)ib*f;
        a += (float)ia*f;
      }
    }
    int ir = clamp_byte((int)r);
    int ig = clamp_byte((int)g);
    int ib = clamp_byte((int)b);
    int ia = clamp_byte((int)a);
    dst->data[y*w + x] = RGBA(ir,ig,ib,ia);
  }
}

void gfx_set_blit_quad(gfx_t *gfx
                      ,int x0, int y0, int x1, int y1
                      ,int x2, int y2, int x3, int y3)
{
  int *q = blt->quad;
  q[0] = x0;
  q[1] = y0;
  q[2] = x1;
  q[3] = y1;
  q[4] = x2;
  q[5] = y2;
  q[6] = x3;
  q[7] = y3;
  blt->bflags |= GFX_BF_QUAD;
}

static void rgb2hsv(float r, float g, float b,
                    float *h, float *s, float *v)
{
    float K = 0.f;
    r /= 255.0f;
    g /= 255.0f;
    b /= 255.0f;
    if (g < b)
    {
        SWAP(g, b);
        K = -1.f;
    }
    float min_gb = b;
    if (r < g)
    {
        SWAP(r, g);
        K = -2.f / 6.f - K;
        min_gb = MIN(g, b);
    }
    float chroma = r - min_gb;
    *h = fabs(K + (g - b) / (6.f * chroma + 1e-20f))*360.0f;
    *s = chroma / (r + 1e-20f);
    *v = r;
}

static float hue_dist(float a, float b) {
  float r;
  if (b < a) {
    float t = a;
    a = b;
    b = t;
  }
  r = b - a;
  if (r > 180.0f) r = 360.0 + a - b;
  return r;
}

static void make_recolor_map_hue(gfx_t *gfx, uint32_t hue, uint32_t range) {
  int i, j, r, g, b, sr, sg, sb, sl;
  float h,s,v, sh,ss,sv;
  int end = gfx->w*gfx->h;
  int nrecolors = gfx->nrecolors;
  uint32_t *recolors = gfx->recolors + gfx->nrecolors;
  uint32_t *d = gfx->data;
  uint8_t *rm = gfx->rmap;
  UNRGB(r,g,b,hue);
  rgb2hsv((float)r,(float)g,(float)b,&h,&s,&v);
  for (i = 0; i < end; i++) {
    uint32_t c = d[i];
    if ((d[i]&0xFF000000) == 0xFF000000) continue;
    rm[i] = 0;
    UNRGB(sr,sg,sb,c);
    rgb2hsv((float)sr,(float)sg,(float)sb,&sh,&ss,&sv);
    if (hue_dist(sh, h) <= (float)range) {
      sr = dgam.glut[sr];
      sg = dgam.glut[sg];
      sb = dgam.glut[sb];
      sl = LUMA8(sr,sg,sb);
      sl = dgam.iglut[int2cfp(sl)];
      if (sl) {
        rm[i] = sl;
      }
    }
  }
}

void gfx_recolorable(gfx_t *gfx, uint32_t hue, uint32_t range) {
  int i = 0;
  uint32_t *recolors;
  if (gfx->rmap) gfx_free_rmap(gfx);
  gfx_alloc_rmap(gfx, 256);
  recolors = gfx->recolors + gfx->nrecolors;
  for (i = 1; i < 257; i++) {
    recolors[i-1] = RGB(i,i,i);
  }
  make_recolor_map_hue(gfx, hue, range);
}

static void make_recolor_map(gfx_t *gfx) {
  int i, j;
  int end = gfx->w*gfx->h;
  int nrecolors = gfx->nrecolors;
  uint32_t *recolors = gfx->recolors + gfx->nrecolors;
  uint32_t *d = gfx->data;
  uint8_t *rm = gfx->rmap;
  for (i = 0; i < end; i++) {
    uint32_t c = d[i]&0xFFFFFF;
    rm[i] = 0;
    for (j = 0; j < nrecolors; j++) {
      if (c == recolors[j]) {
        rm[i] = j+1;
        break;
      }
    }
  }
}

uint32_t *gfx_recolorable_xs_begin(gfx_t *gfx, int nrecolors) {
  int i;
  if (gfx->rmap) gfx_free_rmap(gfx);
  gfx_alloc_rmap(gfx, nrecolors);
  return gfx->recolors + gfx->nrecolors;
}

void gfx_recolorable_xs_end(gfx_t *gfx) {
  make_recolor_map(gfx);
}

uint32_t *gfx_recolor_xs(gfx_t *gfx, int nrecolors) {
  if (nrecolors != gfx->nrecolors) {
    fprintf(stderr,
        "gfx_recolor_xs: recolors number (%d) != recolor places count (%d)\n",
        nrecolors, gfx->nrecolors);
    abort();
  }
  blt->bflags |= GFX_BF_RECOLOR;
  blt->recolored = blt->brecolored-1; //-1 is because rmap has it +1
  return blt->brecolored;
}

//recolor process could be greatly speedup, if we allow user to cache its result
void gfx_recolor(gfx_t *gfx, uint32_t color, float brightness) {
  uint32_t i, r, g, b, sr, sg, sb, l, sl;
  int nrecolors = gfx->nrecolors;
  int br = (int)(brightness*(float)0x100);
  uint32_t *recolors = gfx->recolors;
  UNRGB(r,g,b,color);
  r = dgam.glut[r];
  g = dgam.glut[g];
  b = dgam.glut[b];
  l = LUMA8(r,g,b);
  if (!l) l = 1;
  r = (r<<8)/l;
  g = (g<<8)/l;
  b = (b<<8)/l;
  if (br != 0x100) {
    r = (r*br)>>8;
    g = (g*br)>>8;
    b = (b*br)>>8;
  }
  for (i = 0; i < nrecolors; i++) {
    UNRGB(sr,sg,sb,recolors[i+nrecolors]);
    sr = dgam.glut[sr];
    sg = dgam.glut[sg];
    sb = dgam.glut[sb];
    sl = LUMA8(sr,sg,sb);
    sr = (r*sl)>>8;
    sg = (g*sl)>>8;
    sb = (b*sl)>>8;
    sr = dgam.iglut[int2cfp(sr)];
    sg = dgam.iglut[int2cfp(sg)];
    sb = dgam.iglut[int2cfp(sb)];
    recolors[i] = RGB(sr,sg,sb);
  }
  memcpy(blt->brecolored,gfx->recolors,gfx->nrecolors*sizeof(uint32_t));
  blt->recolored = blt->brecolored-1;
  blt->bflags |= GFX_BF_RECOLOR;
}

void *gfx_get_recolored(gfx_t *gfx) {
  uint32_t *recolored = malloc(gfx->nrecolors*sizeof(uint32_t));
  memcpy(recolored, gfx->recolors, gfx->nrecolors*sizeof(uint32_t));
  return recolored;
}

void gfx_set_recolored(gfx_t *gfx, void *recolored) {
  if (!gfx->rmap) {
    fprintf(stderr, "gfx_set_recolored: gfx has no ready recolor.");
    abort();
  }
  blt->recolored = (uint32_t*)recolored - 1; //-1 is because rmap has it +1
  blt->bflags |= GFX_BF_RECOLOR;
}

int gfx_is_recolorable(gfx_t *gfx) {
  return gfx->rmap != 0;
}

void gfx_set_blit_saturation(gfx_t *gfx, float factor) {
  blt->saturation = factor;
  blt->bflags |= GFX_BF_SATURATE;
}

static uint32_t margins_result[4];

//determines sprite boundaries inside alpha=255 empty area;
//used for packing sprites.
void *gfx_margins(gfx_t *gfx) {
  int w = gfx->w;
  int h = gfx->h;
  uint32_t *d = gfx->data;
  int x1 = w;
  int x2 = -1;
  int sx = 0;
  int xb = w;
  int xe = 0;
  int yb = h;
  int ye = 0;
  int x;
  int y;

  GMUTEX

  for (y = 0; y < h; y++) {
    sx = y*w;
    xb = w;
    xe = -1;
    for (x = 0; x < w; x++) {
      if ((d[x+sx]>>24) != 255) {
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

//moved here to speedup spell of mastery map generation
void gfx_outline_cut(gfx_t *gfx, uint32_t off, uint32_t on) {
  int w = gfx->w;
  int h = gfx->h;
  uint32_t *d = gfx->data;
  int x,y,k;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      uint32_t c = gfx_get(gfx, x, y);
      if (c & 0x8FFFFFF) break;
      gfx_set(gfx, x, y, 0x1000000);
    }
    for (x = w-1; x > -1; x--) {
      uint32_t c = gfx_get(gfx, x, y);
      if (c & 0x9FFFFFF) break;
      gfx_set(gfx, x, y, 0x1000000);
    }
  }
  for (x = 0; x < w; x++) {
    for (y = 0; y < h; y++) {
      uint32_t c = gfx_get(gfx, x, y);
      if (c & 0x8FFFFFF) break;
      gfx_set(gfx, x, y, 0x2000000);
    }
    for (y = y-1; y > -1; y--) {
      uint32_t c = gfx_get(gfx, x, y);
      if (c & 0xAFFFFFF) break;
      gfx_set(gfx, x, y, 0x2000000);
    }
  }
  int repeat = 1;
  int repcnt = 0;
  while(repeat) {
    repeat = 0;
    ++repcnt;
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
        uint32_t c = gfx_get(gfx, x, y);
        if (c & 0x7000000) {
          for (k = x-1; k > 0 && !(gfx_get(gfx,k,y)&0xFFFFFFFF); k--) {
            repeat = 1;
            gfx_set(gfx, k, y, 0x4000000);
          }
          for (k = x+1; k < w && !(gfx_get(gfx,k,y)&0xFFFFFFFF); k++) {
            repeat = 1;
            gfx_set(gfx, k, y, 0x4000000);
          }
          for (k = y-1; k > 0 && !(gfx_get(gfx,x,k)&0xFFFFFFFF); k--) {
            repeat = 1;
            gfx_set(gfx, x, k, 0x4000000);
          }
          for (k = y+1; k < h && !(gfx_get(gfx,x,k)&0xFFFFFFFF); k++) {
            repeat = 1;
            gfx_set(gfx, x, k, 0x4000000);
          }
        }
      }
    }
  }
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      uint32_t c = gfx_get(gfx, x, y);
      uint32_t nc = (c & 0xFF000000) ? off : on;
      // Was: gfx_set(gfx, x, k, nc); -- `k` was leaked from an earlier
      // loop above. Painted random rows. Fixed: write the pixel we
      // just computed back to (x,y). See ui_speedup.md (C7).
      gfx_set(gfx, x, y, nc);
    }
  }
}


uint32_t gfx_count(gfx_t *gfx, uint32_t color) {
  uint32_t *d = gfx->data;
  uint32_t *e = d + gfx->w * gfx->h;
  uint32_t r = 0;
  for ( ; d < e; d++) {
    if (*d == color) r++; 
  }
  return r;
}

static float rand_rng(float rmin, float rmax) {
  return (((float)rand() / (float)RAND_MAX) * (rmax - rmin)) + rmin;
}

//midpoint displacement
static float mpdisp(float a, float b, float c, float d
                   ,float amp, float clip) {
  float r = (a+b+c+d)/4.0 + rand_rng(-amp, amp);
  if (r < clip) r = clip;
  return r;
}

void gfx_make_heightmap(gfx_t *gfx, gfx_t *stencil
                       ,int expt, float smooth
                       ,uint32_t seed) {
  float border_elev = -1.0;
  int w = gfx->w;
  int h = gfx->h;
  int sw = stencil->w;
  int sh = stencil->h;
  int md = 2;
  for (int i = 0; i < expt-1; i++) md *= 2;
  float *map1 = malloc(sizeof(float)*md*md);
  float *map2 = malloc(sizeof(float)*md*md);
  uint32_t pseed = rand(); //save previous seed
  srand(seed);
#define sget(w,h,x,y) gfx_get(stencil,(x)*sw/(w),(y)*sh/(h))
#define mget(r,m,d,x,y) do { \
    int tx = (x)%(d);          \
    int ty = (y)%(d);          \
    if (tx < 0) tx = d + tx; \
    if (ty < 0) ty = d + ty; \
    if (sget((d),(d),tx,ty)) r = border_elev; \
    else r = m[ty*(d) + tx]; \
  } while (0)
#define mset(m,d,x,y,v) do { \
    int tx = (x)%(d);          \
    int ty = (y)%(d);          \
    if (tx < 0) tx = (d) + tx; \
    if (ty < 0) ty = (d) + ty; \
    if (sget((d),(d),tx,ty)) m[ty*(d) + tx] = INFINITY; \
    else m[ty*(d) + tx] = v; \
  } while (0)
  float e = 1.0;
  int d = 2;
  float amp = 1.0;
  map1[0] = rand_rng(-amp, amp);
  map1[1] = rand_rng(-amp, amp);
  map1[2] = rand_rng(-amp, amp);
  map1[3] = rand_rng(-amp, amp);
  for (int i = 0; i < expt-1; i++) {
    int nd = d*2;
    // Was: memset(map2, nd*nd*sizeof(float), 0); -- value/size args
    // swapped. The map was zeroed by the explicit mset() pass below
    // so the bug was latent, but still: fixed. See ui_speedup.md (C7).
    memset(map2, 0, nd*nd*sizeof(float));
    for (int y = 0; y < d; y++) {
      for (int x = 0; x < d; x++) {
        float v;
        mget(v,map1,d,x,y);
        int x2=x*2, y2=y*2;
        mset(map2,nd,x2,y2,v);
      }
    }
    for (int y = 0; y < nd; y++) {
      for (int x = 0; x < nd; x++) {
        if ((x&1) + (y&1) != 2) continue;
        float c0,c1,c2,c3;
        mget(c0,map2,nd,x-1,y-1);
        mget(c1,map2,nd,x+1,y-1);
        mget(c2,map2,nd,x-1,y+1);
        mget(c3,map2,nd,x+1,y+1);
        float mpd = mpdisp(c0, c1, c2, c3, amp, border_elev);
        mset(map2,nd,x,y,mpd);
      }
    }
    for (int y = 0; y < nd; y++) {
      for (int x = 0; x < nd; x++) {
        if ((x&1) + (y&1) != 1) continue;
        float up,dn,lt,rt;
        mget(up,map2,nd,x  ,y-1);
        mget(dn,map2,nd,x  ,y+1);
        mget(lt,map2,nd,x-1,y  );
        mget(rt,map2,nd,x+1,y  );
        float mpd = mpdisp(up, dn, lt, rt, amp, border_elev);
        mset(map2,nd,x,y,mpd);
      }
    }
    float *t = map1;
    map1 = map2;
    map2 = t;
    d = nd;
    amp /= smooth;
  }
  float minv = INFINITY;
  float maxv = -INFINITY;
  int mapn = d*d;
  for (int i = 0; i < mapn; i++) {
    float v = map1[i];
    if (v < minv) minv = v;
    if (v > maxv && v != INFINITY) maxv = v;
  }
  //normalize
  float mm = maxv-minv;
  for (int i = 0; i < mapn; i++) {
    if (map1[i] == INFINITY) map1[i] = minv;
    float v = map1[i];
    map1[i] = (map1[i]-minv)/mm;
  }
  for (int y = 0; y < h; y++) {
    int my = y*d/h;
    for (int x = 0; x < w; x++) {
      if (sget(w,h,x,y)) {
        gfx_set(gfx,x,y,0xFF000000);
      } else {
        int mx = x*d/w;
        float v;
        mget(v,map1,d,mx,my);
        uint32_t c = (int)roundf(v*255.0);
        gfx_set(gfx,x,y,c);
      }
    }
  }
  free(map1);
  free(map2);
  srand(pseed);
}

static void hsv2rgb(float h, float s, float v
                   ,int *dr, int *dg, int *db) {
	float r, g, b;

	if (s == 0) {
		r = v;
		g = v;
		b = v;
	} else {
		int i;
		float f, p, q, t;

		if (h == 360)	h = 0;
		else h /= 60;

		i = (int)trunc(h);
		f = h - i;

		p = v * (1.0 - s);
		q = v * (1.0 - (s * f));
		t = v * (1.0 - (s * (1.0 - f)));

		switch (i) {
		case 0:
			r = v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = v;
			b = p;
			break;
		case 2:
			r = p;
			g = v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = v;
			break;
		case 4:
			r = t;
			g = p;
			b = v;
			break;
		default:
			r = v;
			g = p;
			b = q;
			break;
		}
	}
	*dr = clamp_byte(r * 255);
	*dg = clamp_byte(g * 255);
	*db = clamp_byte(b * 255);
}

void gfx_hue(gfx_t *gfx, int target_hue) {
  int x, y;
  int w = gfx->w;
  int h = gfx->h;
  float th = target_hue;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      int r,g,b,a,nr,ng,nb;
      float sh,ss,sv;
      uint32_t c = gfx_get(gfx, x, y);
      UNRGBA(r,g,b,a,c);
      rgb2hsv((float)r,(float)g, (float)b, &sh, &ss, &sv);
      hsv2rgb(th, ss, sv, &nr, &ng, &nb);
      uint32_t nc = RGBA(nr,ng,nb,a);
      gfx_set(gfx, x, y, nc);
    }
  }
}

void gfx_colorize(gfx_t *gfx, uint32_t target) {
  int x, y;
  int tr, tg, tb;
  int w = gfx->w;
  int h = gfx->h;
  UNRGB(tr,tg,tb,target);
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      int r,g,b,a,nr,ng,nb;
      float sh,ss,sv;
      uint32_t c = gfx_get(gfx, x, y);
      UNRGBA(r,g,b,a,c);
      int l = LUMA8(r,g,b);
      nr = tr*l/255;
      ng = tg*l/255;
      nb = tb*l/255;
      uint32_t nc = RGBA(nr,ng,nb,a);
      gfx_set(gfx, x, y, nc);
    }
  }
}

gfx_t *gfx_rle(gfx_t *gfx) {
  int cur, prev, run;
  int nruns;
  int x, y, w=gfx->w, h=gfx->h;
  uint16_t *rs, *pr, **rle;
  uint8_t *rm, *prmap;
  uint32_t *d, *pdata;
  gfx_t *out;
  int opaq;
  int nopaq = 0;

  if (gfx->flags & GFX_F_RLE) return 0;
  if (w < 24) return 0; //not worth it

  GMUTEX

  if ((gfx->flags&GFX_F_OCT)==GFX_OCT_UNKNOWN) {
    gfx_determine_opacity(gfx);
  }

  rs = malloc((gfx->w+1)*sizeof(uint16_t));
  rle = malloc(h*sizeof(uint16_t*));

  d = gfx->data;

  for (y = 0; y < h; y++) {
    pr = rs;
    prev = 1;
    run = 0;
    for (x = 0; x < w; x++, d++) {
      cur = ((*d>>24) == 0xFF) ? 1 : 0;
      if (cur == prev) run++;
      else {
        if (prev == 0) nopaq += run;
        *pr++ = run;
        run = 1;
        prev = cur;
      }
    }
    if (prev == 0) nopaq += run;
    *pr++ = run;
    nruns = pr-rs;
    rle[y] = malloc(sizeof(uint16_t)*(nruns+2));
    memcpy(rle[y]+2, rs, sizeof(uint16_t)*nruns);
  }
  free(rs);

  if ((float)nopaq/(float)(w*h) > 0.9) {
    for (y = 0; y < h; y++) free(rle[y]);
    free(rle);
    return 0;
  }

  out = new_rle_gfx(gfx, rle, nopaq);
  pdata = out->data;
  prmap = out->rmap;
  d = gfx->data;
  rm = gfx->rmap;
  for (y = 0; y < h; y++) {
    pr = out->rle[y];
    *(uint32_t*)pr = pdata - out->data;
    pr += 2;
    opaq = 0;
    for (x = 0; x < w; x += *pr++) {
      if (opaq) {
        memcpy(pdata, d, *pr*sizeof(uint32_t));
        pdata += *pr;
        if (gfx->rmap) {
          memcpy(prmap, rm, *pr*sizeof(uint8_t));
          prmap += *pr;
        }
      }
      d += *pr;
      if (gfx->rmap) rm += *pr;
      opaq = !opaq;
    }
    run = 0;
  }
  return out;
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

  GMUTEX
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

#if 0
uint32_t array[] = {123,456,789};

void *test(char *name, int x, float y) {
  fprintf(stderr, "%d:%f: Hello, %s! %d,%d,%d\n", x, y, name, array[0], array[1], array[2]);
  return (void*)array;
}

#include "../../../runtime/flt16.h"

static int test_ready = 0;
static gfx_t* bg;
static gfx_t* fgp;
static gfx_t* fgr;

void blit_bench_init() {
  bg = gfx_load_png("./test/bg1.png");
  fgp = gfx_load_png("./test/fg1.png");
  fgr = gfx_rle(fgp);
  //gfx_to_luv(bg);
  //gfx_to_luv(fgp);
}


void *blit_benchp(int step, int niters) {
  int x, y, w, h;

  if (UNLIKELY(!test_ready)) blit_bench_init();

  w = bg->w;
  h = bg->h;

  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      gfx_blit(bg, x, y, fgp);
    }
  }

  return bg;
}

void *blit_benchr(int step, int niters) {
  int x, y, w, h;

  if (UNLIKELY(!test_ready)) blit_bench_init();

  w = bg->w;
  h = bg->h;

  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      gfx_blit(bg, x, y, fgr);
    }
  }

  return bg;
}


/*
make && ../../../gauge/gauge 100 1 blit_benchr ./lib/main

1:
Plain: 0.563
RLE: 0.382
FPS: 122-126

2:
RLE: 0.375
FPS: 126-132

3:
RLE: 0.295

4:
RLE: 0.293
FPS: 113 in editor; 60 ingame

5:
RLE: 0.197
FPS: 128 in editor; 63 ingame

64-bit:
FPS 105 in editor; 32 ingame


*/

#include <immintrin.h>


#include <unistd.h> //sleep


int main(int argc, char **argv) {
  int i;
  int x,y;
  float rle_on, rle_off;
  gfx_t *gfx, *bg, *fg, *rfg, *light;


#if 0
  __m128 xs = _mm_set_ps(4,3,2,1);
  __m128 ys = _mm_set_ps(2,3,4,5);
  __m128 rs = _mm_pow_ps(xs,ys);
  float o[4];
  _mm_storeu_si128((__m128i*)o, rs);
  printf("%f,%f,%f,%f\n", o[3],o[2],o[1],o[0]);
  exit(-1);
#endif

#if 0
  static uint8_t mask[] = { 0xff,  12, 0xff, 0xff
                          , 0xff,  12, 0xff, 0xff
                          , 0xff,  12, 0xff, 0xff
                          , 0xff,  12, 0xff, 0xff};
  __m128 srgb = _mm_setr_epi32(1,2,3,4);
  //__m128 srgb = _mm_set1_epi32(0x04030201);
  //__m128 drgb = _mm_shuffle_epi8(srgb,*(__m128i*)mask);
  __m128 drgb = _mm_set1_epi32(4<<8);
  __m128 result = drgb;
  int32_t o[4];
  __m128i mask2 = _mm_set_epi8(0xff, 11, 10, 9
                              ,0xff, 7, 6, 5
                              ,0xff, 3, 2, 1
                              ,12, 8, 4, 0);
  //result = _mm_shuffle_epi8(result,mask2);
  _mm_storeu_si128((__m128i*)o, result);

  printf("%x,%x,%x,%x\n", o[0],o[1],o[2],o[3]);
  exit(-1);
#endif

#if 0
  bg = gfx_load_png("./test/bg1.png");
  fg = gfx_load_png("./test/fg1.png");
  gfx_set_blit_quad(fg, 100,   0
                      , 200, 100
                      ,   0, 100
                      , 100, 200);
#if 0
  gfx_set_blit_quad(fg,  10,  10
                      , 200,  10
                      ,  10, 100
                      , 200, 200);
#endif
  gfx_blit(bg, 0, 0, fg);
  gfx_save_png("./out.png", bg);
#endif

#if 0
  bg = new_gfx(500, 500);
  fg = gfx_load_png("./test/grid2.png");
  gfx_set_blit_quad(fg,   0, 100
                      , 500,   0
                      ,   0, 400
                      , 500, 500);

  gfx_blit(bg, 0, 0, fg);
  gfx_save_png("./out.png", bg);
#endif

#if 0
  bg = new_gfx(500, 500);
  fg = gfx_load_png("./test/grid2.png");
  gfx_set_blit_quad(fg,   0,   0
                      , 500, 100
                      ,   0, 500
                      , 500, 400);

  gfx_blit(bg, 0, 0, fg);
  gfx_save_png("./out.png", bg);
#endif


#if 0
  bg = new_gfx(500, 500);
  fg = gfx_load_png("./test/grid2.png");
  gfx_set_blit_quad(fg,   0,   0
                      , 500,   0
                      , 100, 500
                      , 400, 500);

  gfx_blit(bg, 0, 0, fg);
  gfx_save_png("./out.png", bg);
#endif

#if 0
  bg = new_gfx(500, 500);
  fg = gfx_load_png("./test/grid2.png");
  gfx_set_blit_quad(fg, 100,   0
                      , 400,   0
                      ,   0, 500
                      , 500, 500);

  gfx_blit(bg, 0, 0, fg);
  gfx_save_png("./out.png", bg);
#endif

#if 0
  bg = new_gfx(500, 500);
  fg = gfx_load_png("./test/grid2.png");
  gfx_set_blit_quad(fg, 100, 100
                      , 400, 100
                      , 100, 400
                      , 400, 400);

  gfx_blit(bg, 0, 0, fg);
  gfx_save_png("./out.png", bg);
#endif


#if 0
  //bg = gfx_load_png("./test/bg3.png");
  //fg = gfx_load_png("./test/fg2.png");
  bg = gfx_load_png("./test/bg1.png");
  fg = gfx_load_png("./test/fg1.png");
  light = gfx_load_png("./test/light.png");
  //gfx_set_blit_saturation(fg, 0.6);
  //gfx_set_blit_colorize(fg, 0, 0.5, 0.5, 3.0);
  gfx_set_blit_stencil(fg, 0x80FFFFFF);
  //gfx_set_blit_fade(fg, 127);
  gfx_set_gamma(fg, 1.0f);
  gfx_blit(bg, 200, 200, fg);
  //gfx_set_blit_fade(light, 0x7f);
  gfx_set_blit_light(light, 3.0, 0xFFFFFF);
  //gfx_set_blit_additive(light, 1.0, 0xFF7F7F);
  //gfx_blit(bg, 100, 100, light);
  gfx_save_png("./out.png", bg);
#endif

#if 1
  //bg = gfx_load_png("./test/bg1.png");
  bg = gfx_load_png("./test/bg3.png");
  //bg = new_gfx(512, 512);
  //gfx_clear(bg,0xFF000000);
  fg = gfx_load_png("./test/fg2.png");
  fg = gfx_rle(fg);
  //gfx_set_blit_bright(fg, -255);

  //gfx_blit(bg, 200, 200, fg);

  gfx_set_bflags_flip_x(fg);
  gfx_blit(bg, 200, 200, fg);

  gfx_set_bflags_flip_x(fg);
  gfx_blit(bg, 438, 200, fg);

  gfx_set_bflags_flip_x(fg);
  gfx_blit(bg, -30, 200, fg);

  gfx_set_bflags_flip_x(fg);
  gfx_blit(bg, 200, -30, fg);

  gfx_set_bflags_flip_x(fg);
  gfx_blit(bg, 200, 338, fg);
  
  GMUTEX

  gfx_save_png("./out.png", bg);
#endif

#if 0
  gfx = gfx_load_png("/Users/macbook/Downloads/example/test/test3.png");
  for (y = 0; y < gfx->h; y++) for (x = 0; x < gfx->w; x++) {
    uint32_t rgb = gfx_get(gfx, x, y);
    uint32_t luv = rgb2luv(rgb);
    luv = luv_saturate(0.5, luv);
    luv = luv_brightness(1.5, luv);
    luv = luv_rotate(0.1, luv);
    gfx_set(gfx, x,y, luv2rgb(luv));
  }
  gfx_save_png("./luv_out.png", gfx);
#endif

#if 0
  if (argc != 3) {
    printf("Usage: %s <infile> <outfile>\n", argv[0]);
    return -1;
  }
  gfx = gfx_load_png(argv[1]);
  if (!gfx) return -1;*/
  //fprintf(stderr, "%dx%d\n", gfx->w, gfx->h);
  gfx_to_luv(gfx);
  for (y = 0; y < gfx->h; y++) for (x = 0; x < gfx->w; x++) {
    uint32_t luv = gfx_get(gfx, x, y);
    //luv = luv_rotate(0.5, luv);
    //luv = luv_saturate(0.5, luv);
    gfx_set(gfx, x,y, luv);
  }
  gfx_to_rgb(gfx);
  gfx_save_png(argv[2], gfx);
#endif

#if 0
  gfx = new_gfx(256, 256);
  for (y = 0; y < 256; y++) for (x = 0; x < 256; x++) {
    uint32_t rgb = luv2rgb((f2h(1.0)<<16) + (y<<8) + x);
    uint32_t luv = rgb2luv(rgb);
    rgb = luv2rgb(luv);
    gfx_set(gfx, x,y, rgb);
  }
  gfx_save_png("./luv_spectrum.png", gfx);
#endif

  fprintf(stderr, "done!\n");

  return 0;
}
#else
int main(int argc, char **argv) {
  return 0;
}
#endif
