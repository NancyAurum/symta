#include <stdio.h>
#include <stdint.h>

#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "stb_truetype.h"


#include "../../../runtime/effi.h"
#include "../../gfx/src/gfx.h"


effi_t *rt;

//FIXME: font unloading
extern void *ttf_load(char *filename) {
  int64_t ttf_size;
  uint8_t *ttf_buffer = rt->fs_get(&ttf_size, filename);
  stbtt_fontinfo *font = malloc(sizeof(stbtt_fontinfo));
  stbtt_InitFont(font, ttf_buffer, stbtt_GetFontOffsetForIndex(ttf_buffer,0));
  //free(ttf_buffer);
  
  return font;
}


gfx_t *(*pnew_gfx)(uint32_t w, uint32_t h);

extern void *ttf_render(void *pfont, int code, float height, uint32_t rgb) {
  int w, h, ox, oy;
  stbtt_fontinfo *font = (stbtt_fontinfo *)pfont;
  float scalex = 0;
  float scaley = stbtt_ScaleForPixelHeight(font, height);
  //float scaley = stbtt_ScaleForMappingEmToPixels(font, height);
  //float scaley = 1.0;
  uint8_t *pbm = stbtt_GetCodepointBitmap(font, scalex, scaley, code
                                         ,&w, &h, &ox, &oy);

  gfx_t *gfx = pnew_gfx(w,h);
  gfx->x = ox;
  gfx->y = oy;

  int cr,cg,cb;
  UNRGB(cr,cg,cb,rgb); 
  
  for (int y=0; y < h; ++y) {
    for (int x=0; x < w; ++x) {
#if 1
      //reduces font blurness a bit
      int a = 0xFF - pbm[y*w+x]*3/2;
      if (a < 0) a = 0;
#else
      int a = 0xFF - pbm[y*w+x];
#endif
      gfx->data[y*w+x] = RGBA(cr,cg,cb,a);
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
