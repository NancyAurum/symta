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
  GMUTEX
  for (; i < e; i++) {
    d[i] = color;
  }
}

//transparent hline
static void gfx_ahline(gfx_t *gfx, uint32_t color, int x, int y, int length) {
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
  GMUTEX
  int sr, sg, sb, sa, ra;
  UNRGBA(sr,sg,sb,sa,color);
  ra = 255 - sa;
  sr *= sa;
  sg *= sa;
  sb *= sa;
  for (; i < e; i++) {
    uint32_t dc = d[i];
    int dr, dg, db, da;
    int cr, cg, cb;
    UNRGBA(dr,dg,db,da,dc);
    cr = (dr*ra + sr)>>8;
    cg = (dg*ra + sg)>>8;
    cb = (db*ra + sb)>>8;
    d[i] = RGBA(cr,cg,cb,da);
  }
}

static void gfx_ihline(gfx_t *gfx, uint32_t color, int x, int y, int length) {
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
  GMUTEX
  for (; i < e; i++) {
    uint32_t c = d[i];
    uint32_t a = c&0xFF000000;
    d[i] = a|((0xFFFFFFFF-c)&0xFFFFFF);
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
  GMUTEX
  for (; i < e; i+=w) {
    d[i] = color;
  }
}


void gfx_rect(gfx_t *gfx, uint32_t color, int fill, int x, int y, int w, int h) {
  scissor_t *p = gfx->scissor;
  if (!w || !h) return;
  if (p) {
    int px = p->rect[0];
    int py = p->rect[1];
    int pex = px + p->rect[2];
    int pey = py + p->rect[3];
    int cx = MIN(pex,MAX(px,x));
    int cy = MIN(pey,MAX(py,y));
    int ex = x+w;
    int ey = y+h;
    int cex = MAX(px,MIN(pex,ex));
    int cey = MAX(py,MIN(pey,ey));
    int cw = cex-cx;
    int ch = cey-cy;
    if (!cw || !ch) return;
    if (fill) {
      if (fill == 1) {
        if (color & 0xFF000000) {
          fprintf(stderr, "hello\n");
          for (; cy < cey; cy++) gfx_ahline(gfx, color, cx, cy, cw);
        } else {
          for (; cy < cey; cy++) gfx_hline(gfx, color, cx, cy, cw);
        }
      } else {
        for (; cy < cey; cy++) gfx_ihline(gfx, color, cx, cy, cw);
      }
    } else {
      if (cy == y) gfx_hline(gfx, color, cx, cy, cw);
      if (cey == ey) gfx_hline(gfx, color, cx, cy+ch - 1, cw);
      gfx_vline(gfx, color, cx, cy, ch);
      gfx_vline(gfx, color, cx+cw-1, cy, ch);
    }
  } else {
    if (fill) {
      int ey = y+h;
      if (fill == 1) {
        if (color & 0xFF000000) {
          for (; y < ey; y++) gfx_ahline(gfx, color, x, y, w);
        } else {
          for (; y < ey; y++) gfx_hline(gfx, color, x, y, w);
        }
      } else {
        for (; y < ey; y++) gfx_ihline(gfx, color, x, y, w);
      }
    } else {
      gfx_hline(gfx, color, x, y, w);
      gfx_hline(gfx, color, x, y+h - 1, w);
      gfx_vline(gfx, color, x, y+1, h-2);
      gfx_vline(gfx, color, x+w-1, y+1, h-2);
    }
  }
}

static void gfx_circle_empty(gfx_t *gfx, uint32_t color, int x, int y, int r) {
  int p = 1 - r;
  int px = 0;
  int py = r;
  GMUTEX
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
  int dw = gfx->w, dh = gfx->h;
  lerp l, r;

  if(ax < 0 && bx < 0 && cx < 0) return;
  if(ax >= dw && bx >= dw && cx >= dw) return;
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

  if(end_y == beg_y || end_y < 0 || beg_y >= dh) return;

  if (beg_y < 0) {
    lerp_init(&r, ax, cx, -beg_y, end_y-beg_y);
    y = 0;
  } else {
    lerp_init(&r, ax, cx,      0, end_y-beg_y);
    y = beg_y;
  }

  if (y < cen_y) {
    if (beg_y < 0) {
      lerp_init(&l, ax, bx, -beg_y, cen_y-beg_y);
    } else {
      lerp_init(&l, ax, bx,      0, cen_y-beg_y);
    }

    if (cen_y > dh) e = dh;
    else e = cen_y;

    for (; y < e; ++y) {
      TRIANGLE_ROW(l,r);
      lerp_advance(&l);
      lerp_advance(&r);
    }
  }

  if(cen_y < end_y) {
    lerp_init(&l, bx, cx, y-cen_y, end_y-cen_y);
    if (end_y > dh) end_y = dh;
    
    for (; y < end_y; ++y) {
      TRIANGLE_ROW(l,r);
      lerp_advance(&l);
      lerp_advance(&r);
    }
  }
}
#undef TRIANGLE_ROW