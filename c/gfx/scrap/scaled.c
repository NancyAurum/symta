void gfx_scaled(gfx_t *gfx, gfx_t *src) {
  GMUTEX
  uint32_t *p = gfx->data;
  uint32_t *sdata = src->data;
  uint32_t w = gfx->w;
  uint32_t h = gfx->h;
  uint32_t sw = src->w;
  uint32_t sh = src->h;
  int x,y;
  for (y = 0; y < gfx->h; y++) {
    uint32_t *ps = sdata + (y*sh/h)*sw;
    for (x = 0; x < gfx->w; x++) {
      int sx = x*sw/w;
      *p++ = ps[sx];
    }
  }
  gfx->flags &= ~(uint32_t)GFX_F_OCT;
}