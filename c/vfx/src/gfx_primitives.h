#include <gfx.h>

#:common

uint32_t gfx_get(gfx_t *gfx, int x, int y);
void gfx_set(gfx_t *gfx, int x, int y, uint32_t color);
void gfx_line(gfx_t *gfx, uint32_t color, int sx, int sy, int dx, int dy);
void gfx_rect(gfx_t *gfx, uint32_t color, int fill, int x, int y, int w, int h);

uint32_t *gfx_margins(gfx_t *gfx, int invert);
