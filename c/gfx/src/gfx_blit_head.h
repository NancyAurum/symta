#ifdef B_RECOLOR
uint8_t * RESTRICT rmap = src->rmap;
uint32_t * RESTRICT recolors = pbl->recolored;
#endif

#if defined B_BLEND || defined B_BRIGHTEN || defined B_SATURATE
gfx_gamma_t *gm = src->gamma;
uint8_t * RESTRICT piglut = gm->iglut;
#endif

#if defined B_BLEND || defined B_BRIGHTEN
uint32_t (* RESTRICT pab_lut)[256] = gm->ab_lut;
uint32_t (* RESTRICT pfab_lut)[256] = gm->fab_lut;
#endif

#if defined B_SATURATE
int sat_factor = (int)(256.0f*pbl->saturation);
uint32_t *RESTRICT glut = gm->glut;
#endif

#ifdef B_LIGHT
int lr, lg, lb; //light filter
UNRGB(lr,lg,lb,pbl->light_color);
lr = (int)(lr*pbl->light_energy);
lg = (int)(lg*pbl->light_energy);
lb = (int)(lb*pbl->light_energy);
#endif

#ifdef B_BRIGHTEN
uint8_t *RESTRICT pr_br = pbl->r_br;
uint8_t *RESTRICT pg_br = pbl->g_br;
uint8_t *RESTRICT pb_br = pbl->b_br;
uint8_t *RESTRICT pa_br = pbl->a_br;
#endif

