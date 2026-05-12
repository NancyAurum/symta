#ifndef GFX_H
#define GFX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define GFX_BF_BLEND        0x01
#define GFX_BF_BRIGHTEN     0x02
#define GFX_BF_GBLEND       0x04
#define GFX_BF_RECOLOR      0x08
#define GFX_BF_SATURATE     0x10
#define GFX_BF_RLE          0x20
#define GFX_BF_ADDITIVE     0x40
#define GFX_BF_LIGHT        (GFX_BF_ADDITIVE|GFX_BF_BLEND)
#define GFX_BF_ERASER       (GFX_BF_GBLEND)

#define GFX_BF_FLIP_X       0x10000
#define GFX_BF_FLIP_Y       0x20000
#define GFX_BF_RECT         0x40000
#define GFX_BF_GAMMA        0x80000
#define GFX_BF_QUAD         0x100000
//#define GFX_BF_SCISSOR      0x200000

#define GFX_F_GBLEND        GFX_BF_GBLEND
#define GFX_F_RLE           GFX_BF_RLE

//opacity types
#define GFX_OCT_UNKNOWN      0x0
#define GFX_OCT_BLEND        0x1
#define GFX_OCT_OPAQUE       0x20000
#define GFX_OCT_BINARY       0x40000
#define GFX_F_OCT           (GFX_OCT_BLEND|GFX_OCT_OPAQUE|GFX_OCT_BINARY)

typedef struct { // gamma lookup tables
  //inverse gamma lookup table
  //bigger than 0x10000, because cfp mantissa could have more bits than expected
  uint8_t iglut[0x20000];
  uint32_t glut[0x100]; //rgb to linear lookup table
  //alpha blending lookup tables
  uint32_t ab_lut[256][256];
  uint32_t fab_lut[256][256];
  float value; //gamma value
} gfx_gamma_t;

typedef struct scissor_t scissor_t;

struct scissor_t {
  int rect[4]; //scissors rect
  scissor_t *next;
};

typedef struct {
  uint32_t w; // width
  uint32_t h; // height
  uint32_t x; // x offset for blitting
  uint32_t y; // y offset for blitting
  uint16_t **rle; // rle optimization data for each y
  uint8_t *rmap; // recolor map/mask
  uint32_t *recolors; // recolors source/destination
  uint32_t nrecolors; //size of recolors
  uint32_t flags; //flags describing image
  gfx_gamma_t *gamma; // custom gamma lookup tables
  scissor_t *scissor;
  uint32_t data[];
} gfx_t;

gfx_t *new_gfx(uint32_t w, uint32_t h);
void free_gfx(gfx_t *gfx);
uint32_t gfx_get(gfx_t *gfx, int x, int y);
void gfx_set(gfx_t *gfx, int x, int y, uint32_t color);
void gfx_clear(gfx_t *gfx, uint32_t color);
void gfx_determine_opacity(gfx_t *gfx);
void gfx_blit(gfx_t *gfx, int x, int y, gfx_t *src);
void gfx_triangle(gfx_t *gfx, uint32_t color
                 , int ax, int ay, int bx, int by, int cx, int cy);

void gfx_scissor(gfx_t *gfx, int x, int y, int w, int h);

void gfx_quad(gfx_t *dst, gfx_t *src, int *quad);

void gfx_gamma_init(gfx_gamma_t *gamma, double value);

void gfx_sync(gfx_t *gfx);

gfx_t *gfx_load_png(char *filename);
void gfx_save_png(char *filename, gfx_t *gfx);

//non-rentrant versions for internal use
void _free_gfx(gfx_t *gfx);
void _gfx_clear(gfx_t *gfx, uint32_t color);
void _gfx_blit(gfx_t *gfx, int x, int y, gfx_t *src);


#define unless(x) if(!(x))
#define times(i,e) for(i=0; i<(e); i++)

#define RGB(R,G,B) (((R)<<16)|((G)<<8)|(B))
#define RGBA(R,G,B,A) (((A)<<24)|((R)<<16)|((G)<<8)|(B))

#define UNRGBNA(R,G,B,C) do { \
  uint32_t _fromC = (C); \
  B = (((_fromC)    )&0xFF); \
  G = (((_fromC)>> 8)&0xFF); \
  R = (((_fromC)>>16)); \
 } while (0)

#define UNRGB(R,G,B,C) do { \
  uint32_t _fromC = (C); \
  B = (((_fromC)    )&0xFF); \
  G = (((_fromC)>> 8)&0xFF); \
  R = (((_fromC)>>16)&0xFF); \
 } while (0)
#define UNRGBA(R,G,B,A,C) do { \
  uint32_t _fromC = (C); \
  B = (((_fromC)    )&0xFF); \
  G = (((_fromC)>> 8)&0xFF); \
  R = (((_fromC)>>16)&0xFF); \
  A = (((_fromC)>>24)); \
 } while (0)
#define UNR5G6B5(R,G,B,C) do { \
  uint32_t _fromC = (C)&0xFFFF; \
  B = ((((_fromC)>> 0)&0x1f)<<3)|0x7; \
  G = ((((_fromC)>> 5)&0x3f)<<2)|0x3; \
  R = ((((_fromC)>>11)&0x1f)<<3)|0x7; \
 } while (0)
#define UNR5G5B5A1(A,R,G,B,C) do { \
  uint32_t _fromC = (C)&0xFFFF; \
  A = (_fromC)&1; \
  B = ((((_fromC)>> 1)&0x1f)<<3)|0x7; \
  G = ((((_fromC)>> 6)&0x1f)<<3)|0x7; \
  R = ((((_fromC)>>11)&0x1f)<<3)|0x7; \
 } while (0)
#define UNA1R5G5B5(A,R,G,B,C) do { \
  uint32_t _fromC = (C)&0xFFFF; \
  B = ((((_fromC)>> 0)&0x1f)<<3)|0x7; \
  G = ((((_fromC)>> 5)&0x1f)<<3)|0x7; \
  R = ((((_fromC)>>10)&0x1f)<<3)|0x7; \
  A = ((_fromC)>>15)&1; \
 } while (0)
#define UNA4R4G4B4(A,R,G,B,C) do { \
  uint32_t _fromC = (C)&0xFFFF; \
  B = ((((_fromC)>> 0)&0xf)<<4)|0xf; \
  G = ((((_fromC)>> 4)&0xf)<<4)|0xf; \
  R = ((((_fromC)>> 8)&0xf)<<4)|0xf; \
  A = ((((_fromC)>>12)&0xf)<<4)|0xf; \
  B |= B>>4; \
  G |= G>>4; \
  R |= R>>4; \
  A |= A>>4; \
 } while (0)


#endif
