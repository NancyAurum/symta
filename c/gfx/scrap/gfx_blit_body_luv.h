do {
sa = m[s-src->data];
#ifdef B_FADE
sa += fade_amount;
#endif
#if !defined B_RLE || defined B_FADE
if (sa >= 0xFF) break;
#endif
#ifdef B_RECOLOR
c = rmap[s-src->data];
c = c ? recolors[c] : SC;
#else
c = SC;
#endif
#if defined B_BLEND || defined B_BRIGHTEN || defined B_SATURATE|| defined B_FADE
UNLUV(sl,su,sv,c);
fsl = h2f(sl);
#else
DC = c;
dm[pd] = 0;
#endif
#ifdef B_BRIGHTEN
fsl += br_boost;
#endif
#ifdef B_SATURATE
do {
  int f = sat_factor;
  sv = LUV_WV + ((sv - LUV_WV)*f>>8);
  su = LUV_WU + ((su - LUV_WU)*f>>8);
  sv = clamp_byte(sv);
  su = clamp_byte(su);
} while(0);
#endif
#ifdef B_BLEND
if (sa == 0) {
  DC = LUV(f2h(fsl),su,sv);
  dm[pd] = 0;
} else if (dm[pd]) {
  if (dm[pd]==0xFF) {
    DC = LUV(f2h(fsl),su,sv);
    dm[pd] = sa;
  } else  {
    int wd,ws;
    UNLUV(dl,du,dv,DC);
    da = dm[pd];
    ra = (da * sa)>>8;
    ws = 255 - sa;
    wd = sa - ra;
    st = luv_fab_lut[ws];
    dt = luv_fab_lut[wd];
    fsl = (h2f(dl)*(float)wd + fsl*(float)ws)*rdiv_lut[ra];
    div = ridiv_lut[ra];
    sv = ((dt[dv] + st[sv])*div)>>8;
    su = ((dt[du] + st[su])*div)>>8;
    DC = LUV(f2h(fsl),su,sv);
    dm[pd] = ra;
  }
} else {
  UNLUV(dl,du,dv,DC);
  fsa = (float)sa*(1.0f/255.0f);
  fsl = fsa*h2f(dl) + (1.0-fsa)*fsl;
  sv = (((dv - sv)*sa)>>8) + sv;
  su = (((du - su)*sa)>>8) + su;
  DC = LUV(f2h(fsl),su,sv);
}
#else //B_BLEND
#if defined B_BRIGHTEN || defined B_SATURATE
  DC = LUV(f2h(fsl),su,sv);
  dm[pd] = 0;
#endif
#endif //B_BLEND
} while (0);
