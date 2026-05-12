#ifndef LUV_H
#define LUV_H

#include <stdint.h>

//whitepoint
#define LUV_WV 0x7F
#define LUV_WU 0x4A


void luv_init();

//convert color from RGB to a LUV triplet.
//assumes alpha byte is set to 0
uint32_t rgb2luv(uint32_t luv);

//convert color from LUV to a RGB triplet.
//assumes alpha byte is set to 0
uint32_t luv2rgb(uint32_t rgb);

//rgb2luv unpacked
void rgb2luv_u(int ir, int ig, int ib, float *rl, int *ru, int *rv);

//luv2rgb unpacked
void luv2rgb_u(float l, int iu, int iv, int *rr, int *rg, int *rb);

//boosts or lowers saturation of a color
//negative values are also accepted, turning color into opposite
uint32_t luv_saturate(float s, uint32_t luv);


uint32_t luv_rotate(float a, uint32_t luv);

uint32_t luv_brightness(float s, uint32_t luv);

#endif
