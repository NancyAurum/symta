#include <stdio.h>
#include <stdint.h>

#define NANOSVG_IMPLEMENTATION
#include "./nsvg/src/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "./nsvg/src/nanosvgrast.h"

#include "../../../runtime/effi.h"
#include "../../gfx/src/gfx.h"


effi_t *rt;

extern void *svg_load(char *filename) {
  uint8_t *svg_buffer = rt->fs_get(0, filename);
	NSVGimage *svg = nsvgParse((char*)svg_buffer, "px", 96);
  return svg;
}

extern void svg_free(NSVGimage *svg) {
  nsvgDelete(svg);
}


gfx_t *(*pnew_gfx)(uint32_t w, uint32_t h);

extern gfx_t *svg_render(NSVGimage *svg, int wsize, int hsize) {
	struct NSVGrasterizer* rast = nsvgCreateRasterizer();
  int sw = (int)svg->width, sh = (int)svg->height;
  int w,h;
  float scale;
  if (wsize) {
    w = wsize;
    h = (sh*wsize+sw-1)/sw;
    scale = (float)wsize/sw;
  } else if (hsize) {
    w = (sw*hsize+sh-1)/sh;
    h = hsize;
    scale = (float)hsize/sh;
  } else {
    w = sw;
    h = sh;
    scale = 1.0;
  }
  gfx_t *gfx = pnew_gfx(w,h);
  memset(gfx->data, 0, w*h*4);
	nsvgRasterize(rast, svg, 0,0,scale, (uint8_t*)gfx->data, w, h, w*4);
  nsvgDeleteRasterizer(rast);

  uint32_t *p = gfx->data;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int r,g,b,a;
      UNRGBA(r,g,b,a,*p);
      *p = RGBA(b,g,r,255-a);
      p++;
    }
  }
  return gfx;
}

extern void *synit(effi_t *s) {
  rt = s;
  void *pgfx = rt->ffi_load("gfx", "new_gfx");
  pnew_gfx = (gfx_t *(*)(uint32_t w, uint32_t h))pgfx;
  return 0;
}
