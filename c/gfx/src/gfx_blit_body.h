#if defined B_GBLEND && !defined B_BLEND
#define B_ERASER
#endif
do {
#if !defined B_RLE || defined B_BRIGHTEN || defined B_BLEND || defined B_LIGHT || defined B_ERASER
int sa = *s>>24;
#endif
#ifdef B_BRIGHTEN
sa = pa_br[sa];
#endif
#if !defined B_RLE || defined B_BRIGHTEN
if (sa == 0xFF) break;
#endif
#ifdef B_RECOLOR
uint32_t c = rmap[s-src->data];
if (!c) {
  c = SC;
} else {
  c = recolors[c];
#if !defined B_RLE || defined B_BRIGHTEN || defined B_BLEND || defined B_LIGHT
  c = c|(sa<<24);
#endif
}
#else
uint32_t c = SC;
#endif
#if defined B_BLEND||defined B_LIGHT || defined B_BRIGHTEN || defined B_SATURATE
int sr, sg, sb; //source color components
UNRGB(sr,sg,sb,c);
#else
DC = c;
#endif
#ifdef B_SATURATE
do {
  int f = sat_factor;
  int r = glut[sr];
  int g = glut[sg];
  int b = glut[sb];
  int l = LUMA8(r,g,b)*(256-f);
  sr = piglut[int2cfpL((r*f + l)>>8)];
  sg = piglut[int2cfpL((g*f + l)>>8)];
  sb = piglut[int2cfpL((b*f + l)>>8)];
} while(0);
#endif
#ifdef B_BRIGHTEN
sr = pr_br[sr];
sg = pg_br[sg];
sb = pb_br[sb];
#endif
#if defined B_LIGHT
{
  int dr, dg, db, da;
  UNRGBA(dr,dg,db,da,DC);
  sr = sr*(255-sa)>>8;
  sg = sg*(255-sa)>>8;
  sb = sb*(255-sa)>>8;
#ifdef B_BLEND
  //multiplicative light
  dr = clamp_byte255(dr+((dr*sr*lr)>>16));
  dg = clamp_byte255(dg+((dg*sg*lg)>>16));
  db = clamp_byte255(db+((db*sb*lb)>>16));
#else
  //additive light
  dr = clamp_byte255(dr+((sr*lr)>>8));
  dg = clamp_byte255(dg+((sg*lg)>>8));
  db = clamp_byte255(db+((sb*lb)>>8));
#endif
  DC = RGBA(dr,dg,db,da);
}
#elif defined B_ERASER //!defined B_LIGHT
{
  int dr, dg, db, da;
  UNRGBA(dr,dg,db,da,DC);
  da = clamp_byte255(da+255-sa);
  DC = RGBA(dr,dg,db,da);
}
#else //!defined B_LIGHT && !defined B_ERASER
#if defined B_BLEND || defined B_BRIGHTEN
if (sa == 0) {
  DC = RGB(sr,sg,sb);
} else if (UNLIKELY(DC>>24)) {
  if ((DC>>24)==0xFF) {
    DC = RGBA(sr,sg,sb,sa);
  } else  {
    int ra; //result alpha
    int dr, dg, db, da;
    uint32_t div, * RESTRICT st, * RESTRICT dt;
    UNRGBA(dr,dg,db,da,DC);
    ra = (da * sa)>>8;
    div = ridiv_lut[ra];
    st = pfab_lut[255 - sa];
    dt = pfab_lut[sa - ra];
    sr = piglut[int2cfpL((dt[dr] + st[sr])*div)];
    sg = piglut[int2cfpL((dt[dg] + st[sg])*div)];
    sb = piglut[int2cfpL((dt[db] + st[sb])*div)];
    DC = RGBA(sr,sg,sb,ra);
  }
} else {
#ifdef B_GBLEND
#ifdef X86_SSE
  ALIGN(16) static uint8_t iamsk[] = {
    0xff, 12, 0xff, 0xff
  , 0xff, 12, 0xff, 0xff
  , 0xff, 12, 0xff, 0xff
  , 0xff, 12, 0xff, 0xff
  };
  //__m128 msa = _mm_set1_epi32(sa<<8);
  #if defined B_BRIGHTEN || defined B_SATURATE
  __m128 srgb = _mm_setr_epi32(sb,sg,sr,sa);
  #else
  __m128 srgb = _mm_shuffle_epi8(_mm_set1_epi32(c),sse_imsk);
  #endif
  __m128 msa = _mm_shuffle_epi8(srgb,*(__m128i*RESTRICT)iamsk);
  __m128 drgb = _mm_shuffle_epi8(_mm_set1_epi32(DC),sse_imsk);
  __m128 r1 = _mm_mulhi_epu16(_mm_sub_epi16(drgb,srgb), msa);
  __m128 r2 = _mm_add_epi16(r1, srgb);
  DC = _mm_extract_epi32(_mm_shuffle_epi8(r2,sse_omsk),0);
#else //X86_SSE
  int dr, dg, db;
  UNRGBNA(dr,dg,db,DC);
  dr = (((dr-sr)*sa)>>8) + sr;
  dg = (((dg-sg)*sa)>>8) + sg;
  db = (((db-sb)*sa)>>8) + sb;
  DC = RGB(dr,dg,db);
#endif //X86_SSE
#else
  uint32_t *RESTRICT st, *RESTRICT dt;
  int dr, dg, db;
  UNRGBNA(dr,dg,db,DC);
  st = pab_lut[255-sa];
  dt = pab_lut[sa];
  sr = piglut[int2cfpL(st[sr]+dt[dr])];
  sg = piglut[int2cfpL(st[sg]+dt[dg])];
  sb = piglut[int2cfpL(st[sb]+dt[db])];
  DC = RGB(sr,sg,sb);
#endif
}
#else //B_BLEND
#if defined B_SATURATE
  DC = RGB(sr,sg,sb);
#endif
#endif //B_BLEND
#endif //B_LIGHT
} while (0);
#if defined B_GBLEND && !defined B_BLEND
#undef B_ERASER
#endif