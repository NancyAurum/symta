//extracted from the gfx library
#:gfx_primitives
#:slb

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

static uint32_t margins_result[4];

//determines sprite boundaries inside alpha=255 empty area;
//used for packing sprites.
uint32_t *gfx_margins(gfx_t *gfx, int invert) {
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

  for (y = 0; y < h; y++) {
    sx = y*w;
    xb = w;
    xe = -1;
    for (x = 0; x < w; x++) {
      int filled = invert ? (d[x+sx]>>24) == 255 : (d[x+sx]>>24) != 255;
      if (filled) {
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

#if 0
static void gfx_invert_alpha(gfx_t *gfx) {
  uint32_t *d = gfx->data;
  uint32_t *ed = d + gfx->h*gfx->w;
  for ( ; d < ed; d++) {
    *d = (*d&0x00FFFFFF) | (0xFF000000 - (*d&0xFF000000));
  }
}
#endif