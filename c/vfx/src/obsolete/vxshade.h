/*
ffi vxShadeIllum.void Flags.u4
                         AR.float AG.float AB.float
                         LR.float LG.float LB.float
                         LX.float LY.float LZ.float

ffi vxShade.void Gfx.ptr Scale.float CanvasScale.float
                 CBuf.ptr ZBuf.ptr NBuf.ptr

gfx.shaded Scale CanvasScale CBuf ZBuf NBuf Ambient Intensity Light
           shading/emboss flags/[] =
| FR = ShadingMethods.Shading
| when no FR: bad "unknown shading method: [Shading]"
| for F Flags:
  | V = ShadingFlags.F
  | when got V: FR += V
| vxShadeIllum FR
      Ambient Ambient Ambient
      Intensity Intensity Intensity
      Light.0 Light.1 Light.2
| $pixels
| vxShade $handle Scale CanvasScale CBuf.handle ZBuf.handle NBuf.handle

/*| when $shading><flat:
  | $fb.blit{0 0 $cbuf}
  | $fbs.scaled{$fb}
  | leave Me
| CS = $vxz.canvas_scale
| Light = [$w.float*0.5 $h.float*0.5 -($w+$h).float*1.0]
| Ambient = 0.5
//| Intensity = 0.5
| Intensity = 1.5
//| Intensity = 2.0
| Shading = $shading
| when Shading><default: Shading <= \emboss
| $fb.shaded{$scale CS $cbuf $zbuf $nbuf Ambient Intensity Light
              shading/Shading falgs/[smooth]}
| $fbs.scaled{$fb}*/

*/

//slb_shade illumination
typedef struct {
  uint32_t flags;
  vec3 ambient;   //ambient light color
  vec3 point; //point light x,y,z
  vec3 color; //point light color
} sil_t;

static sil_t sil;

void vxShadeIllum(uint32_t flags
    ,float ar, float ag, float ab
    ,float lr, float lg, float lb
    ,float lx, float ly, float lz)
{
  sil.flags = flags;
  sil.ambient.x = ar;
  sil.ambient.y = ag;
  sil.ambient.z = ab;
  sil.point.x = lx;
  sil.point.y = ly;
  sil.point.z = lz;
  sil.color.x = lr;
  sil.color.y = lg;
  sil.color.z = lb;
}



static float awm;
static float ahm;
static int shd_lowfi;

static INLINE
vec3 zbuf_normal(uint32_t x0, uint32_t y0, float z0
                ,uint32_t x1, uint32_t y1, float z1
                ,uint32_t x2, uint32_t y2, float z2)
{
  vec3 o, a, b, n, ldir;
  if (shd_lowfi) {
    o = (vec3){(float)x0,(float)y0,(float)((uint32_t)(z0*30))};
    a = (vec3){(float)x1,(float)y1,(float)((uint32_t)(z1*30))};
    b = (vec3){(float)x2,(float)y2,(float)((uint32_t)(z2*30))};
  } else {
    //FIXME: X and Y should be devided by viewport W and H
    //       as of now all viewports are 256x256
    o = (vec3){(float)x0*awm,(float)y0*ahm,(float)z0};
    a = (vec3){(float)x1*awm,(float)y1*ahm,(float)z1};
    b = (vec3){(float)x2*awm,(float)y2*ahm,(float)z2};
  }
  a -= o;
  b -= o;
  if (shd_lowfi==2) {
    VFLOOR32(a,a);
    VFLOOR32(b,b);
    VFLOOR32(o,o);
  }
  n = cross(a,b);
  return normalized(n);
}

uint32_t *generate_normals_from_zbuffer(uint32_t w, uint32_t h
                                       ,float *depths, uint32_t flags) {
  uint32_t x,y;
  uint32_t *normals = malloc(sizeof(uint32_t)*w*h);
  for (y = 1; y < h; y++) { //generate normals from the z-buffer values
    uint32_t *pns = normals + y*w;
    float *pds = depths + y*w;
    float *ppds = depths + (y-1)*w;
    float *npds = depths + (y+1)*w;
    for (x = 1; x < w; x++) {
      float nc = pds[x];
      float nx = pds[x-1];
      float ny = ppds[x];
      vec3 n = zbuf_normal(x,y,nc,x-1,y,nx,x,y-1,ny);
      if ((flags&SHD_SMOOTH) && x<w-1 && y<h-1) {
        float nnx = pds[x+1];
        float nny = npds[x];
        vec3 n2 = zbuf_normal(x,y,nc,x+1,y,nnx,x,y+1,nny);
        n = (n+n2)*0.5;
      }
      pns[x] = vnormal2rgb(n);
    }
  }
  return normals;
}

void vxShade(gfx_t *out, float scale
              ,float canvas_scale, gfx_t *cbuf, zbuf_t *zbuf, gfx_t *nbuf) {
  uint32_t r,g,b,a,x,y, nr,ng,nb;
  uint32_t w = cbuf->w;
  uint32_t h = cbuf->h;
  uint32_t flags = sil.flags;
  uint32_t method = SHD_TYPE(flags);
  uint32_t *colors = cbuf->data;
  uint32_t *out_data = out->data;
  uint32_t *normals = 0;
  float intensity = (sil.color.x + sil.color.y + sil.color.z)/3.0;
  float ambient = (sil.ambient.x + sil.ambient.y + sil.ambient.z)/3.0;
  float min_intensity = -ambient*0.5;
  //float min_intensity = -ambient;
  vec3 light = sil.point;
  light.y = -light.y;
  uint32_t lowfi = 0;
  awm = 1.0/out->w;
  ahm = 1.0/out->h;
  //flags |= SHD_LOWFI;
  //flags |= SHD_GLOW;
  //flags |= SHD_PILLOW;
  if (flags&SHD_LOWFI) {
    lowfi = 1;
    light.z *= -4.0;
    ambient *= 0.5;
    if (flags&SHD_GLOW) {
      lowfi = 2;
    }
  }
  shd_lowfi = lowfi;
  
  if (method == SHD_TEST) method = SHD_EMBOSS;

  float *depths = malloc(sizeof(float)*w*h);
  float factor = canvas_scale*8.0;
  float max_dist = sqrt((float)(factor*factor*3))*2.0+0.00001;
  for (y = 0; y < h; y++) { //normalize depths
    //FIXME: this can be eliminated with a proper light in 3d space
    uint32_t *pcs = colors + y*w;
    float *pds = depths + y*w;
    float *src_pds = zbuf->data + y*w;
    for (x = 0; x < w; x++) {
      if (pcs[x]&0xFFFFFF) {
        float depth = src_pds[x];
        depth = (max_dist-depth)*255.0/max_dist;
        if ((flags&SHD_PILLOW) && x && y) {
           depth = ((pds-w)[x]+pds[x-1]+depth)/3; //pillow shading
        }
        pds[x] = depth*scale;
      } else {
        pds[x] = 0.0;
      }
    }
  }

  if (method == SHD_EMBOSS) {
    normals = generate_normals_from_zbuffer(w,h,depths,flags);
  }

  if (method == SHD_ZBUFFER) {
    for (y = 1; y < h; y++) {
      uint32_t *outp = out_data + y*w;
      uint32_t *pcs = colors + y*w;
      float *pds = depths + y*w;
      for (x = 1; x < w; x++) {
        uint32_t color = pcs[x];
        if (!(color&0xFFFFFF)) continue;
        uint32_t d = (pds[x]-90.0f)*255.0f/20.0f;
        //fprintf(stderr, "%f\n", pds[x]);
        UNRGBA(r,g,b,a,color);
        uint32_t dr = clamp_byte(d);
        uint32_t dg = clamp_byte(d);
        uint32_t db = clamp_byte(d);
        outp[x] = RGB(dr,dg,db);
      }
    }
    goto end;
  }

  if (method == SHD_NBUFFER) {
    for (y = 1; y < h; y++) {
      uint32_t *outp = out_data + y*w;
      uint32_t *pcs = colors + y*w;
      //uint32_t *pns = normals + y*w;
      uint32_t *pns = nbuf->data + y*w;
      for (x = 1; x < w; x++) {
        uint32_t color = pcs[x];
        if (!(color&0xFFFFFF)) continue;
        outp[x] = pns[x];
      }
    }
    goto end;
  }

  if (flags&SHD_ZCUE) {
    for (y = 1; y < h; y++) {
      uint32_t *outp = out_data + y*w;
      uint32_t *pcs = colors + y*w;
      float *pds = depths + y*w;
      for (x = 1; x < w; x++) {
        uint32_t color = pcs[x];
        if (!(color&0xFFFFFF)) continue;
        uint32_t d = (pds[x]-90.0f)*255.0f/20.0f;
        //fprintf(stderr, "%f\n", pds[x]);
        UNRGBA(r,g,b,a,color);
        uint32_t dr = clamp_byte((d*d*r)>>16);
        uint32_t dg = clamp_byte((d*d*g)>>16);
        uint32_t db = clamp_byte((d*d*b)>>16);
        outp[x] = RGB(dr,dg,db);
      }
    }
    goto end;
  }

  if (method == SHD_POINTS) {
    intensity = 0.5;
    ambient *= 0.5;
  }

  for (y = 1; y < h; y++) {
    uint32_t *outp = out_data + y*w;
    uint32_t *pcs = colors + y*w;
    float *pds = depths + y*w;
    uint32_t *pns = normals + y*w;
    uint32_t *pns2 = nbuf->data + y*w;
    for (x = 1; x < w; x++) {
      uint32_t color = pcs[x];
      if (!(color&0xFFFFFF)) continue;
      UNRGBA(r,g,b,a,color);
      float z = pds[x];
      vec3 xyz; //surfac xyz
      if (lowfi) {
        xyz = (vec3){(float)x,(float)y,(float)((uint32_t)(z*30))};
      } else {
        xyz = (vec3){(float)x*awm,(float)y*ahm,(float)z};
      }
      vec3 n;
      if (method == SHD_POINTS) {
        n = vrgb2normal(pns2[x]);
      } else {
        n = vrgb2normal(pns[x]);
      }
      vec3 ldir = light-xyz;
      float angle = vangle(ldir,n);
      float l = angle*intensity;
      if (l < min_intensity) l = min_intensity;
      l += ambient;
      uint32_t rl = clamp_byte((uint32_t)((float)r*l));
      uint32_t gl = clamp_byte((uint32_t)((float)g*l));
      uint32_t bl = clamp_byte((uint32_t)((float)b*l));
      outp[x] = RGB(rl,gl,bl);
    }
  }
end:
  free(depths);
  if (normals) free(normals);
}
