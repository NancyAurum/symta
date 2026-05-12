//produces smooth corner version, but it looks incorrect, becasue cube corned don't get cut inside octree. The only way to do it correctly is by employing distance field
static VXSlab *sdslb;

static int ot_corner_count(otree_t *RESTRICT tree, vec3 p) {
  static vec3 octs[] = {
    { 0.5, 0.5, 0.5},{-0.5, 0.5, 0.5},{ 0.5,-0.5, 0.5},{ 0.5, 0.5,-0.5},
    {-0.5,-0.5,-0.5},{-0.5,-0.5, 0.5},{ 0.5,-0.5,-0.5},{-0.5, 0.5,-0.5}
  };
  float c = sdslb->c;
  float w = sdslb->w+EPS;
  float h = sdslb->h+EPS;
  float d = sdslb->d+EPS;
  int count = 0;
  int i;
  for (i = 0; i < 8; i++) {
    vec3 q = p+octs[i];
    if (q.x < -EPS || q.y < -EPS || q.z < -EPS ||
        q.x >= w || q.y >= h || q.z >= d)
    {
      continue;
    }
    if(OT_VOXEL(ot_getf(tree,c,q))) count++;
  }
  //fprintf(stderr, "%d\n", count);
  return count;
}

//FIXME: pick only the planes coming through existing corners.
static float ot_smooth(otree_t *RESTRICT tree, float rt, vec3 ro, vec3 rd) {
  vec3 corners[] = {
    {0,0,0},{1,0,0},{0,1,0},{0,0,1},
    {1,1,1},{1,1,0},{0,1,1},{1,0,1}
  };
  vec3 planes[][3] = { //planes for the corresponding corners
    {{0,0,1},{0,1,1},{1,0,0}},
    {{0,0,0},{1,0,1},{1,1,0}},
    {{0,0,0},{0,1,1},{1,1,0}},
    {{0,0,0},{1,0,1},{0,1,1}},
    {{0,1,1},{1,1,0},{1,0,1}},
    {{1,1,1},{0,1,0},{1,0,0}},
    {{1,1,1},{0,1,0},{0,0,1}},
    {{1,1,1},{1,0,0},{0,0,1}},
  };

  vec3 pv = rt*rd+ro;

  float t = vlen2(pv-ro);
  vec3 p = vfloor(pv+rd*0.01f);
  float w = sdslb->w;
  float h = sdslb->h;
  float d = sdslb->d;
  int i;
  for (i = 0; i < 8; i++) {
    int cc = ot_corner_count(tree,p+corners[i]);
    if (cc>1) continue;
    vec3 a = p+planes[i][0];
    vec3 b = p+planes[i][1];
    vec3 c = p+planes[i][2];
    vec3 plane_normal = cross(a-c,b-c);
    vec3 hit = ray_plane_hit(ro, rd, a, plane_normal);
    float tt = vlen2(hit-ro);
    if (tt > t) t = tt;
  }
  return sqrtf(t);
}


#define OT_XYZ(x,y,z) (((z)<<2)+((y)<<1)+(x))


static void vxFlipX(otree_t *tree, uint32_t node) {
  uint32_t *octs = lst_get(tree,node).octs;
  int i,j;
  for (i = 0; i < 2; i++) for (j = 0; j < 2; j++) {
    SWAP(octs[OT_XYZ(i,j,0)],octs[OT_XYZ(i,j,1)]);
  }
  for (i = 0; i < 8; i++) {
    uint32_t oct = octs[i];
    if (oct&OT_TERM) continue;
    vxFlipX(tree, oct);
  }
}

static void vxFlipY(otree_t *tree, uint32_t node) {
  uint32_t *octs = lst_get(tree,node).octs;
  int i,j;
  for (i = 0; i < 2; i++) for (j = 0; j < 2; j++) {
    SWAP(octs[OT_XYZ(0,i,j)],octs[OT_XYZ(1,i,j)]);
  }
  for (i = 0; i < 8; i++) {
    uint32_t oct = octs[i];
    if (oct&OT_TERM) continue;
    vxFlipY(tree, oct);
  }
}
static void vxFlipZ(otree_t *tree, uint32_t node) {
  uint32_t *octs = lst_get(tree,node).octs;
  int i,j;
  for (i = 0; i < 2; i++) for (j = 0; j < 2; j++) {
    SWAP(octs[OT_XYZ(i,0,j)],octs[OT_XYZ(i,1,j)]);
  }
  for (i = 0; i < 8; i++) {
    uint32_t oct = octs[i];
    if (oct&OT_TERM) continue;
    vxFlipZ(tree, oct);
  }
}


void vxFlip(VXSlab *slb, int axis) {
  if (axis==0) vxFlipX(&slb->tree, 0);
  else if (axis==1) vxFlipY(&slb->tree, 0);
  else vxFlipZ(&slb->tree, 0);
}



//old vxRender
//FIXME: consider sorting items by the distance towards screen.
void vxRender(gfx_t *cbuf, zbuf_t *zbuf) {
#include "camera_setup.h"

  VPRC(rcnt=0); VPRC(vcnt=0);
#ifdef PROFILE
  double TStart = nano_clock();
#endif

  scene_item_t *RESTRICT scene_elts = scene_list.elts;

  float *depths = zbuf->data;
  uint32_t *colors = cbuf->data;
  uint32_t scx,scy;
  
  if (!scene_list.used) {
    uint32_t *pcs = colors + scy*sw;
    float *pds = depths + scy*sw;
    for (scy = 0; scy < sh; scy++) {
      for (scx = 0; scx < sw; scx++) {
        *pds++ = INFINITY;
        *pcs++ = 0xFF000000;
      }
    }
    return;
  }

  for (scy = 0; scy < sh; scy++) { //iterate over screen x,y
    uint32_t *pcs = colors + scy*sw;
    float *pds = depths + scy*sw;
    sxp = syp;
    for (scx = 0; scx < sw; scx++) {
#if 1
      hit_t hit;
      hit.t = INFINITY;
      hit.voxel = 0xFF000000;
      ray_bvh_hit(0, &hit, sxp, szb);
      *pds = hit.t;
      *pcs = hit.voxel;
#else
      float best_depth = INFINITY;
      uint32_t best_voxel = 0xFF000000;
      vec3 best_hit;
      VXSlab *best_vfx;
      for (int i = 0; i < scene_list.used; i++) {
        scene_item_t *RESTRICT item = &scene_elts[i];
        if (!ray_aabb_hit_test(sxp, szb, item->aabb.min, item->aabb.max)) {
          continue;
        }
#if 0
        *pds = 1000.0f;
        *pcs = 0xFF0000;
        goto next_x;
#endif
        vec3 ro = sxp;
        vec3 rd = szb;
        vec3 rom = vmm(ro,item->view);
        vec3 rdm = (vmm(rd*PRECSCL+ro,item->view) - rom)/PRECSCL;
        rom += item->o;
        float hit_depth; //current voxel hit by screen ray
        VXSlab *vfx = item->vfx;
        uint32_t voxel = raymarch(vfx,&hit_depth,rom,rdm);
        if(!voxel) continue;
        if (hit_depth > best_depth) continue;
        best_depth = hit_depth;
        best_hit = rd*hit_depth + ro;
        best_voxel = voxel;
        best_vfx = vfx;
      }
      *pds = best_depth;
      *pcs = best_voxel;
#endif
    next_x:
      pds++;
      pcs++;
      sxp += s.xb;
    }
    syp -= s.yb;
  }
  VPRC(fprintf(stderr, "vxRender: rays=%d, voxels=%d vpr=%g\n"
                , rcnt, vcnt, (float)vcnt/(float)rcnt));

#ifdef PROFILE
  double TEnd = nano_clock();
  double TPassed = TEnd-TStart; 
  VPRINT(cam.angle);
  fprintf(stderr, "vxRender time: %g seconds\n", TPassed);
#endif
}


void vfx_render_slab(vfx_t *vfx, gfx_t *cbuf, zbuf_t *zbuf) {
  VPRC(rcnt=0); VPRC(vcnt=0);
  camera_t cam = vfx_adjust_camera(vfx, gcam);
#include "sample_head.h"

#ifdef PROFILE
  double TStart = nano_clock();
#endif

  float mscale = vfx->scale;
  float madd = cam_distance*(1.0-mscale);

  float *depths = zbuf->data;
  uint32_t *colors = cbuf->data;
  uint32_t scx,scy;
  for (scy = 0; scy < sh; scy++) { //iterate over screen x,y
    uint32_t *pcs = colors + scy*sw;
    float *pds = depths + scy*sw;
    sxp = syp;
    for (scx = 0; scx < sw; scx++) {
      uint32_t voxel = raymarch(vfx,&hit,sxp,szb);
      if(!voxel) goto next_x;
      float depth = distance(hit,sxp)*mscale + madd;
      if (depth > *pds) goto next_x;
      *pds = depth;
      *pcs = voxel;
    next_x:
      pds++;
      pcs++;
      sxp += s.xb;
    }
    syp -= s.yb;
  }
  VPRC(fprintf(stderr, "vfx_sample: rays=%d, voxels=%d vpr=%g\n"
                , rcnt, vcnt, (float)vcnt/(float)rcnt));

#ifdef PROFILE
  double TEnd = nano_clock();
  double TPassed = TEnd-TStart; 
  VPRINT(cam.angle);
  fprintf(stderr, "vfx_sample time: %g seconds\n", TPassed);
#endif
}


static
uint32_t ot_raymarch(otree_t *RESTRICT tree, vec3 *RESTRICT hit
                    ,uint32_t node, vec3 ro, vec3 rd, vec3 ird, vec3 o, vec3 vc)
{
  int oct;
  uint32_t *octs = lst_get(tree,node).octs;
  oct_hit_t hits[8];
  oct_hit_t *phits[8];
  float next_dist = INF_DIST;
  vec3 next_o;
  vec3 next_hit;
  node = 0;
  int nhits = 0;
  for(oct = 0; oct < 8; oct++) {
    phits[oct] = &hits[oct];
    uint32_t next = octs[oct];
    if (!OT_VOXEL(next)) { //empty
      hits[oct].t = INF_DIST;
      continue;
    }
    //vec3 gt = {(oct&1),(oct>>1)&1,(oct>>2)&1};
    //vec3 to = o + gt*vc;
    vec3 to = o + oct2gt[oct]*vc;
    vec3 hout;
    VPRC(vcnt++);
    float t = ray_cube_hit1(ro, ird, to, vc);
    hits[oct].t = t;
    if (!(t < INF_DIST)) continue;
    nhits++;
    hits[oct].o = to;
    hits[oct].node = next;
  }
  //if (nhits > 4) fprintf(stderr, "%d\n", nhits);
  sort_octs(phits);
  vc *= 0.5;
  for(oct = 0; oct < 8; oct++) {
    oct_hit_t *ph = phits[oct];
    float t = ph->t;
    if (!(t < INF_DIST)) continue;
    uint32_t node = ph->node;
    if (node&OT_TERM) {
      *hit = rd*t + ro;
      return OT_VOXEL(node);
    }
    uint32_t voxel = ot_raymarch(tree, hit, node, ro, rd, ird, ph->o, vc);
    if (voxel) return voxel;
  }
  return 0;
}



static INLINE
uint32_t raymarch(vfx_t *RESTRICT vfx, vec3 *RESTRICT hit, vec3 ro, vec3 rd) {
  vec3 origin = {0,0,0};
  vec3 box = {vfx->dw, vfx->dh, vfx->dd};
  vec3 p, hita, hitb, step, pad;
  //check if hit the scene box
  if (!ray_box_hit(&p, &hitb, ro, rd, origin, box)) return 0;

  VPRC(rcnt++);

  //#define RAY_STEP 0.95f
#define RAY_STEP 0.50f
  //#define RAY_STEP 0.25f
#define RAY_PAD 0.001f
  float d = 0.0f; //distance traveled on the ray
  float end = distance(p,hitb)-RAY_STEP; //margin since dist is not precise
  step = rd * RAY_STEP;
  pad = rd * RAY_PAD;
  termf_t t;
  otree_t *RESTRICT tree = &vfx->tree;
  float dc = vfx->c;

  while (d < end) {
    VPRC(vcnt++);
    //if (vcnt > 1000000) fprintf(stderr, "%f\n", d);
    uint32_t color = ot_get_termf(&t, tree, dc, p)&0xFFFFFF;
    if (color) {
return_result:;
#if 1
      *hit = p; //this gives smoother texture
#else
      int x = (int)p.x;
      int y = (int)p.y;
      int z = (int)p.z;
      *hit = (vec3){(float)x+0.5,(float)y+0.5,(float)z+0.5}; //this gives rough texture
#endif 

#if 0
      slope_hit(hit, vfx, p, ro, rd);
#endif
      return color;
    }

#if 0
    //this affects shading, and should not be used with a few voxels cubes
    //i.e. it must a an option, turned off for small scale models.
    //yet it speeds up significantly.
    if (t.c < 4) do {
      p += step;
      d += RAY_STEP;
      color = ot_get_termf(&t, tree, dc, p)&0xFFFFFF;
      if (color) {
        step *= 0.1f;
        while (1) {
          //walk back in smaller step, ensures we havent hopped a voxel
          p -= step;
          uint32_t tc = ot_getf(tree, dc, p.x, p.y, p.z)&0xFFFFFF;
          if (!tc) goto return_result;
          color = tc;
        }
      }
      if (!(t.c < 4)) break;
      if (!(d < end)) return 0;
    } while(1);
#endif

    //no need to check for return - it always happens here
    ray_cube_hit(&hitb, ro, rd, t.o, t.c);

    /*int GotHit = ray_cube_hit //check if hit the scene box
                        (&hita, &hitb, src, ray_dir
                        , sx,sy,sz,sx+c,sy+c,sz+c);
    
    if (!GotHit) {
      fprintf(stderr, "miss!\n");
    }*/

    float dinc = distance(p,hitb);
    if (dinc < 0.0001) { //important to avoid hangs
      d += RAY_STEP;
      p += step;
    } else {
      d += dinc;
      p = hitb;
      d += RAY_PAD+EPS;
      p += pad;
    }
  }
  return 0;
}









//these are the non-vector versions.
static INLINE
uint32_t ot_get_termf(termf_t *RESTRICT t, otree_t *RESTRICT tree
                     ,float c, vec3 p)
{
  uint32_t node = 0;
  float x=p.x, y=p.y, z=p.z;
  float ox=0, oy=0, oz=0;
  for(;;){
    uint32_t *octs = lst_get(tree,node).octs;
    uint32_t oct;
    vec3 no = ((vec3){x,y,z} >= (vec3){ox,oy,oz}+(vec3){c,c,c})*(vec3){c,c,c};
    if (x >= ox+c) {
      oct = 0x1;
      ox += c;
    } else {
      oct = 0;
    }
    if (y >= oy+c) {oct |= 0x2; oy += c;}
    if (z >= oz+c) {oct |= 0x4; oz += c;}

    node = octs[oct];
    if (node&OT_TERM) {
      t->c = c;
      t->o = (vec3){ox,oy,oz};
      return node;
    }
    c *= 0.5;
  }
}


static INLINE
uint32_t ot_getf(otree_t *RESTRICT tree,float c,float x, float y, float z) {
  uint32_t node = 0;
  for(;;){
    uint32_t *octs = lst_get(tree,node).octs;
    uint32_t oct;
    if (x >= c) {
      oct = 0x1;
      x -= c;
    } else {
      oct = 0;
    }
    if (y >= c) {oct |= 0x2; y -= c;}
    if (z >= c) {oct |= 0x4; z -= c;}

    node = octs[oct];
    if (node&OT_TERM) return node;
    c *= 0.5;
  }
}





// rotate around z-axis by angle a
static INLINE
void rotate3d_x(vec3 *dst, vec3 *src, float a) {
  float c = cos(a);
  float s = sin(a);
  //float y = src->y*c - src->z*s;
  //float z = src->y*s + src->z*c;
  float y = (*src).z*s + (*src).y*c;
  float z = (*src).z*c - (*src).y*s;
  VSET((*dst),(*src).x,y,z);
}

// rotate around y-axis by angle a
static INLINE
void rotate3d_y(vec3 *dst, vec3 *src, float a) {
  float c = cos(a);
  float s = sin(a);
  float x = (*src).x*c + (*src).z*s;
  float z = (*src).z*c - (*src).x*s;
  VSET((*dst),x,(*src).y,z);
}

// rotate around z-axis by angle a
static INLINE
void rotate3d_z(vec3 *dst, vec3 *src, float a) {
  float c = cos(a);
  float s = sin(a);
  float x = (*src).x*c - (*src).y*s;
  float y = (*src).x*s + (*src).y*c;
  VSET((*dst),x,y,(*src).z);
}

//rotate point around origin
static INLINE
void vrotate(vec3 *dst, vec3 *src, vec3 *origin, vec3 *angle) {
  *dst = *src;
  VSUB(*dst,*origin);
  rotate3d_x(dst,dst,(*angle).x);
  rotate3d_y(dst,dst,(*angle).y);
  rotate3d_z(dst,dst,(*angle).z);
  VADD(*dst,*origin);
}

//reverse point rotatation around origin
static INLINE
void vrotate_r(vec3 *dst, vec3 *src, vec3 *origin, vec3 *angle) {
  *dst = *src;
  VSUB(*dst,*origin);
  rotate3d_z(dst,dst,-(*angle).z);
  rotate3d_y(dst,dst,-(*angle).y);
  rotate3d_x(dst,dst,-(*angle).x);
  VADD(*dst,*origin);
}


//rotate plane `src` around rotation origin by angle
static INLINE
void rotate_plane(screen_t *dst, screen_t *plane, vec3 *origin, vec3 *angle) {
  int i;
  vec3 *vs[3];
  VMOV(dst->o ,plane->o);
  VMOV(dst->xb,plane->xb);
  VMOV(dst->yb,plane->yb);
  VADD(dst->xb,dst->o);
  VADD(dst->yb,dst->o);
  vs[0] = &dst->o;
  vs[1] = &dst->xb;
  vs[2] = &dst->yb;
  for (i = 0; i < 3; i++) {
    vec3 *v = vs[i];
    VSUB(*v,*origin);
    rotate3d_x(v,v,angle->x);
    rotate3d_y(v,v,angle->y);
    rotate3d_z(v,v,angle->z);
    VADD(*v,*origin);
  }
  VSUB(dst->xb,dst->o);
  VSUB(dst->yb,dst->o);
}


double vfx_sd(vfx_t *vfx, uint32_t *color, xyz_t p) {
  int i, j;
  xyz_t b, q, o;

#if 1
#define NDS 27
  int ds[][3] = {
     {-1,-1,-1},{-1,-1, 0},{-1,-1, 1},{-1, 0,-1},{-1, 0, 0},{-1, 0, 1}
    ,{-1, 1,-1},{-1, 1, 0},{-1, 1, 1},{ 0,-1,-1},{ 0,-1, 0},{ 0,-1, 1}
    ,{ 0, 0,-1},{ 0, 0, 0},{ 0, 0, 1},{ 0, 1,-1},{ 0, 1, 0},{ 0, 1, 1}
    ,{ 1,-1,-1},{ 1,-1, 0},{ 1,-1, 1},{ 1, 0,-1},{ 1, 0, 0},{ 1, 0, 1}
    ,{ 1, 1,-1},{ 1, 1, 0},{ 1, 1, 1}
  };
#else
#define NDS 6
  int ds[][3] = {
     {-1, 0, 0},{ 0,-1, 0},{ 0, 0,-1},{1, 0, 0},{ 0, 1, 0},{ 0, 0, 1}
  };
#endif


  double sdf;

  termf_t t;
  uint32_t cl = ot_get_termf(&t, &vfx->tree,vfx->c, p.x,p.y,p.z)&0xFFFFFF;
  VSET(b, t.c, t.c, t.c);
  VSET(q, p.x-t.x, p.y-t.y, p.z-t.z);
  sdf = sdBox(q,b);
  if (cl || sdf>1.0) {
    *color = cl;
    return sdf;
  }

  *color = 0;
  sdf = 999999999999.0;
  double step = 1.0;
  double m = step;
  for (j = 0; j < 32; j++) {
    for (i = 0; i < NDS; i++) {
      double dx=ds[i][0]*m, dy=ds[i][1]*m, dz=ds[i][2]*m;
      double xx=p.x+dx, yy=p.y+dy, zz=p.z+dz;
      termf_t t;
      uint32_t cl = ot_get_termf(&t, &vfx->tree,vfx->c, xx,yy,zz)&0xFFFFFF;
      if (!cl) continue;
      VSET(b, t.c, t.c, t.c);
      VSET(q, p.x-t.x, p.y-t.y, p.z-t.z);
      double tsdf = sdBox(q,b);
      if (tsdf < sdf) {
        sdf = tsdf;
        *color = cl;
      }
    }
    if (*color) break;
    m += step;
    //step *= 1.1;
  }
  if (!*color) sdf = 32.0;
  return sdf;
}

uint32_t raymarch(vfx_t *RESTRICT vfx
                 ,xyz_t *RESTRICT hit
                 ,xyz_t *RESTRICT ro_arg
                 ,xyz_t *RESTRICT rd_arg)
{
  int i, j;
  uint32_t color = 0;
  xyz_t r, ro, rd;
  VMOV(ro,*ro_arg);
  VMOV(rd,*rd_arg);
  VMOV(r, ro);

  xyz_t b, o, q;
  VSET(o, vfx->c, vfx->c, vfx->c);
  VSET(b, vfx->c, vfx->c, vfx->c);

  for (i = 0; i < 8; i++) {
    VMOV(q, r);
    //VSUB(q, o);
    //double sd = sdBox(q,b); color = 0xFF0000;
    double sd = vfx_sd(vfx, &color, q);
    //fprintf(stderr, "%d:%g\n", i, sd);
    if (sd < 0.5) {
      if (!color) {
        for (j = 0; j < 8; j++) {
          xyz_t q;
          VMOV(q, rd);
          VMUL(q, 0.25*j);
          VADD(q, r);
          color = vfx_get(vfx, (int)q.x, (int)q.y, (int)q.z);
          if (color) {
            VMOV(r, q);
            break;
          }
        }
      }
      VMOV(*hit, r);
      return color; //0xFF0000;
    }
    xyz_t t;
    VMOV(t, rd);
    VMUL(t, sd);
    VADD(r, t);
  }


  return 0;
}



//FIXME: octs should be looked-up to the leaves
double sdOcts(uint32_t *octs, xyz_t origin, xyz_t p, double c) {
  int i;
  double sdf = 999999999999.0;
  xyz_t b,q;
  VSET(b, c, c, c);
  VMOV(q, p);
  VSUB(q, origin);

  for(i = 0; i < 8; i++) {
    if (!(octs[i]&0xFFFFFF)) continue;
    double dx = i&1;
    double dy = (i>>1)&1;
    double dz = (i>>2)&1;
    xyz_t t;
    VSET(t, q.x-dx*c, q.y-dy*c, q.z-dz*c);
    double tsdf = sdBox(t,b);
    if (tsdf < sdf) {
      sdf = tsdf;
    }
  }
  return sdf;
}

double sdOctsT(uint32_t *octs, xyz_t origin, xyz_t p, double c) {
  int i;
  xyz_t b,q;
  VSET(b, c, c, c);
  VMOV(q, p);
  VSUB(q, origin);
  
  return sdBox(q,b);
}

uint32_t raymarch(vfx_t *RESTRICT vfx
                 ,xyz_t *RESTRICT hit
                 ,xyz_t *RESTRICT ro_arg
                 ,xyz_t *RESTRICT rd_arg)
{
  int i;
  xyz_t r, ro, rd, o;
  VMOV(ro,*ro_arg);
  VMOV(rd,*rd_arg);
  otree_t *RESTRICT tree = &vfx->tree;
  double c = vfx->c;
  uint32_t node = 0;
  int depth = 0;
  VMOV(r, ro);
  //VSET(o, 1.0, 1.0, 2.5);
  //VSET(o, c, c, c);
  VSET(o, 0.0, 0.0, 0.0);
  //VPRINT(r);
  for(;;) {
    uint32_t *octs = lst_get(tree,node).octs;
    depth++;
    double sd;
    for (i = 0; i < 16; i++) {
      xyz_t oo;
      VSET(oo, c,c,c);
      VADD(oo, o);
      sd = sdOcts(octs, oo, r, c);
      if (sd < 0.00001) break;
      xyz_t t;
      VMOV(t, rd);
      VMUL(t, sd);
      VADD(r, t);
    }
    //fprintf(stderr, "depth=%d, c=%g sd=%g\n", depth, c, sd);
    //VPRINT(r);

    /*if (fabs(sd)>1.0) return 0;
    //fprintf(stderr, "%g\n", sd);
    VMOV(*hit, r);
    return 0xFF0000;*/


    /*xyz_t t;
    VMOV(t, rd);
    VMUL(t, 1.0);
    VADD(r, t);*/


    //fprintf(stderr, "depth=%d, c=%g sd=%g\n", depth, c, sd);
    //VPRINT(r);

    xyz_t d;
    VMOV(d, r);
    VSUB(d, o);

    uint32_t oct;
    if (d.x >= c) {
      oct = 0x1;
      o.x += c;
    } else {
      oct = 0;
    }
    if (d.y >= c) {oct |= 0x2; o.y += c;}
    if (d.z >= c) {oct |= 0x4; o.z += c;}

    node = octs[oct];
    if (node&OT_TERM) {
      if (fabs(sd)>1.0) return 0;
      //fprintf(stderr, "0x%x\n", node);
      VMOV(*hit, r);
      return node&0xFFFFFF;
    }
    c *= 0.5;
  }
}


static double sdChunk(vfx_t *vfx, xyz_t center, xyz_t p) {
  int i;
  double sdf = 999999999999.0;
  xyz_t q, b;
  int x = (int)center.x;
  int y = (int)center.y;
  int z = (int)center.z;
  int ds[][3] =
    {{-1,-1,-1},{-1,-1,0},{-1,-1,1},{-1,0,-1},{-1,0,0},{-1,0,1}
    ,{-1,1,-1},{-1,1,0},{-1,1,1},{0,-1,-1},{0,-1,0},{0,-1,1}
    ,{0,0,-1},{0,0,0},{0,0,1},{0,1,-1},{0,1,0},{0,1,1}
    ,{1,-1,-1},{1,-1,0},{1,-1,1},{1,0,-1},{1,0,0},{1,0,1}
    ,{1,1,-1},{1,1,0},{1,1,1}
    /*{ 0, 0, 0}
    ,{ 1, 0, 0},{ 0, 1, 0},{ 0, 0, 1},{-1, 0, 0},{ 0,-1, 0},{ 0, 0,-1}

    ,{ 1, 1, 0},{ 0, 1, 1},{-1,-1, 0},{ 0,-1,-1}
    ,{ 1, 1,-1},{-1, 1, 1},{-1,-1, 1},{ 1,-1,-1}
    ,{ 1, 1, 1},{-1,-1,-1},{ 1,-1, 1},{-1, 1,-1}*/
  };

  VSET(b, 1.0, 1.0, 1.0);
  VMOV(q, p);
  VSUB(q, center);

  for(i = 0; i < 27; i++) {
    int dx=ds[i][0], dy=ds[i][1], dz=ds[i][2];
    int xx=x+dx, yy=y+dy, zz=z+dz;
    uint32_t c = vfx_get(vfx,xx,yy,zz);
    if (!c) continue;
    xyz_t t;
    VSET(t, q.x-dx, q.y-dy, q.z-dz);
    double tsdf = sdBox(t,b,0.0);
    //double tsdf = sdSphere(t,1.0);
    if (tsdf < sdf) {
      sdf = tsdf;
    }
  }
  return sdf - 1.0;
}

static INLINE
void slope_hit(xyz_t *hit, vfx_t *vfx, xyz_t p, xyz_t ro, xyz_t rd) {
  int i;
  int x = (int)p.x;
  int y = (int)p.y;
  int z = (int)p.z;

  xyz_t center;
  VSET(center, (double)x + 0.5, (double)y + 0.5, (double)z + 0.5);
  //VSET(center, (double)x, (double)y, (double)z);

  xyz_t r;
  VMOV(r, ro);
  for (i = 0; i < 8; i++) {
    double sd = sdChunk(vfx, center, r);
    xyz_t t;
    VMOV(t, rd);
    VMUL(t, sd);
    VADD(r, t);
  }
  VMOV(*hit, r);

}




//old half implemented vfx_boolean
void vfx_boolean(vfx_t *vfx, vfx_t *brush) {
  uint32_t invert = 0;
  xyz_t angle;
  xyz_t org; // (0,0,0) origin
  xyz_t xb, yb, zb; //normal 1-diagonal basis matrix

  VSET(org,0.0,0.0,0.0);
  VSET(xb,1.0,0.0,0.0);
  VSET(yb,0.0,1.0,0.0);
  VSET(zb,0.0,0.0,1.0);

  double scale = vfx->scale/brush->scale;

  xyz_t mrot, mrot2;


  VMOV(mrot,brush->angle);
  VMOV(mrot2,vfx->angle);

  VDEG2RAD(mrot, mrot);
  VDEG2RAD(mrot2, mrot2);

  //VSUB(mrot,vfx->angle);

  //VPRINT(mrot);

  //VSET(mrot, 15.0, 30.0, 45.0);


  xyz_t rxm, rym, rzm;

  VMOV(rxm,xb);
  VMOV(rym,yb);
  VMOV(rzm,zb);

  vrotate(&rxm, &rxm, &org, &mrot);
  vrotate(&rym, &rym, &org, &mrot);
  vrotate(&rzm, &rzm, &org, &mrot);

  xyz_t qxm, qym, qzm;
  vrotate_r(&qxm, &xb, &org, &mrot2);
  vrotate_r(&qym, &yb, &org, &mrot2);
  vrotate_r(&qzm, &zb, &org, &mrot2);

  //fprintf(stderr, "hello\n");

  VPRINT(rxm);
  VPRINT(rxm);
  VPRINT(rym);
  VPRINT(rzm);

#define BCNT brush->center
#define VCNT vfx->center
#define BXYZ brush->xyz
#define VXYZ vfx->xyz


  uint32_t x,y,z;
  otree_t *tree = &vfx->tree;
  term_lst_t *terms = vfx_list_terms(vfx);
  for (int i = 0; i < terms->used; i++) {
    term_t *t = &lst_get(terms,i);
    uint32_t color = lst_get(tree,t->node).octs[t->oct]&0xFFFFFF;
    if (!color) continue; //nothing to cut
    uint32_t sx = t->x;
    uint32_t sy = t->y;
    uint32_t sz = t->z;
    uint32_t c = t->c;
    uint32_t cuts_count = 0;
    uint32_t voxel_count = 0;
    for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
      for (x = sx; x < sx+c; x++) {
        int xx=x, yy=y, zz=z;
        double fx=xx-VCNT.x+BXYZ.x,fy=yy-VCNT.y+BXYZ.y,fz=zz-VCNT.z+BXYZ.z;

        double fxx, fyy, fzz;

        //the following gives incorrect results for the just brush rotated 
        /*xx = (int)(rxm.x*fx + rxm.y*fy + rxm.z*fz);
        yy = (int)(rym.x*fx + rym.y*fy + rym.z*fz);
        zz = (int)(rzm.x*fx + rzm.y*fy + rzm.z*fz);*/

        xx = (int)(rxm.x*fx + rym.x*fy + rzm.x*fz);
        yy = (int)(rxm.y*fx + rym.y*fy + rzm.y*fz);
        zz = (int)(rxm.z*fx + rym.z*fy + rzm.z*fz);

        fx=xx,fy=yy,fz=zz;

        /*xx = (int)(qxm.x*fx + qym.x*fy + qzm.x*fz);
        yy = (int)(qxm.y*fx + qym.y*fy + qzm.y*fz);
        zz = (int)(qxm.z*fx + qym.z*fy + qzm.z*fz);*/

        /*xx = (int)(qxm.x*fx + rxm.y*fy + qxm.z*fz);
        yy = (int)(qym.x*fx + qym.y*fy + qym.z*fz);
        zz = (int)(qzm.x*fx + qzm.y*fy + qzm.z*fz);*/

        xx += BCNT.x;
        yy += BCNT.y;
        zz += BCNT.z;
        uint32_t cut = vfx_get(brush, xx, yy, zz);
        if (invert) cut = !cut;
        if (cut) {
          if (c==1) goto do_cut_term;
          cuts_count++;
          if (cuts_count != voxel_count) goto do_split_term;
        }
        continue;
  next_x:
        if (cuts_count) {
          goto do_split_term;
        }
      }
    }
    if (cuts_count) {
      if (cuts_count == voxel_count) {
  do_cut_term:
          lst_get(tree,t->node).octs[t->oct] = 0|OT_TERM;
      } else {
  do_split_term:
        lst_get(tree,t->node).octs[t->oct] = tree->used;
        lst_add(node,tree);
        uint32_t *octs = lst_get(tree,node).octs;
        uint32_t c2 = c>>1;
        for (int i = 0; i < 8; i++) {
          uint32_t ox=sx,oy=sy,oz=sz;
          if (i&0x1) ox += c2;
          if (i&0x2) oy += c2;
          if (i&0x4) oz += c2;
          octs[i] = color|OT_TERM;
          lst_add(term_index,terms);
          term_t *term = &lst_get(terms,term_index);
          term->x = ox;
          term->y = oy;
          term->z = oz;
          term->c = c2;
          term->node = node;
          term->oct = i;
        }
      }
    }
  }
  free_term_lst(terms);
  vfx_remesh(vfx);
}






//obsolete projection code
xyz_t rxw, ryw, rzw; //reverse world rotation
xyz_t rxm, rym, rzm; //reverse model rotation

//TODO: fix broken rotation code!
wrot.x = -wrot.x;
mrot.x = -mrot.x;

vrotate_r(&rxw, &xb, &org, &wrot);
vrotate_r(&ryw, &yb, &org, &wrot);
vrotate_r(&rzw, &zb, &org, &wrot);

vrotate_r(&rxm, &xb, &org, &mrot);
vrotate_r(&rym, &yb, &org, &mrot);
vrotate_r(&rzm, &zb, &org, &mrot);
/* ... */
VSUB(hv, center); //reverse the model rotation around center
hvr.x = rxm.x*hv.x + rym.x*hv.y + rzm.x*hv.z;
hvr.y = rxm.y*hv.x + rym.y*hv.y + rzm.y*hv.z;
hvr.z = rxm.z*hv.x + rym.z*hv.y + rzm.z*hv.z;
VADD(hvr, center);
VSUB(hvr, cam.focus); //reverse the world rotation around focus
xyz_t scp; //voxel p projected onto screen
scp.x = rxw.x*hvr.x + ryw.x*hvr.y + rzw.x*hvr.z;
scp.y = rxw.y*hvr.x + ryw.y*hvr.y + rzw.y*hvr.z;
//scp.z = rxw.z*hvr.x + ryw.z*hvr.y + rzw.z*hvr.z;




static INLINE //obsolete raymarcher
uint32_t raymarch(vfx_t *RESTRICT vfx
                 ,xyz_t *RESTRICT hit
                 ,xyz_t *RESTRICT src
                 ,xyz_t *RESTRICT ray_dir) {
  double dd = vfx->d;
  xyz_t hita, hitb, dir;
  int GotHit = ray_cube_hit
                      (&hita, &hitb, src, ray_dir
                      ,0.0,0.0,0.0,dd,dd,dd); //FIXME: vfx WxHxD should be used
  if (!GotHit) return 0;

  VPRC(rcnt++);

#if 0 //to debug the cube itself
  VMOV(*hit,hita);
  return 0x00FF00|OT_TERM;
#endif

  VMOV(dir,*ray_dir);
  int l = (int)vdist(&hita,&hitb)+1; //add 1, since dist is not precise

  if (dd < 16.0) {
    VMUL(dir,0.1);
    l *= 10;
  }
  term_t t;
  otree_t *RESTRICT tree = &vfx->tree;
  while (l) {
    VPRC(vcnt++);
    int x = (int)hita.x;
    int y = (int)hita.y;
    int z = (int)hita.z;
    //uint32_t color = vfx_get(vfx,x,y,z);
    uint32_t color = ot_get_term(&t, tree, vfx->c, x, y, z)&0xFFFFFF;

    if (!color) {
      l--;
      VADD(hita,dir);
      continue;
    }

    VMUL(dir, 0.1);
    while (1) { //walk back in smaller step, ensures we havent hopped a voxel
      VSUB(hita,dir);
      int tx = (int)hita.x;
      int ty = (int)hita.y;
      int tz = (int)hita.z;
      uint32_t tcolor = vfx_get(vfx,tx,ty,tz);
      if (!tcolor) break;
      color = tcolor;
      x = tx;
      y = ty;
      z = tz;
    }

    //VSET(*hit,(double)x,(double)y,(double)z); //this gives rough texture
    VMOV(*hit,hita); //this gives smoother texture

    return color;
  }
  return 0;
}


//from camera_setup.h
#define right_angle(a) \
  ((a)==0.0 || (a)==(PI*0.5) || (a)==(PI) || (a)==(PI*1.5))
    

#ifdef ANGLE_HACK
  if (right_angle(angle.x)) angle.x += EPS; //hack to fix issues with 45 angle
  if (right_angle(angle.y)) angle.y += EPS;
  if (right_angle(angle.z)) angle.z += EPS;
#endif    


//raymarch with additional astuff
#if 1

static INLINE
uint32_t raymarch(vfx_t *RESTRICT vfx
                 ,xyz_t *RESTRICT hit
                 ,xyz_t *RESTRICT src
                 ,xyz_t *RESTRICT ray_dir) {
  double dd = vfx->d+EPS;
  xyz_t p, hita, hitb, step, pad;
  int GotHit = ray_cube_hit //check if hit the scene box
                      (&p, &hitb, src, ray_dir
                      ,0.0,0.0,0.0,dd,dd,dd); //FIXME: vfx WxHxD should be used
  if (!GotHit) return 0;

  VPRC(rcnt++);

#define RAY_STEP 0.95
  //#define RAY_STEP 0.25
#define RAY_PAD 0.001
  double dist = 0.0;
  double end = vdist(&p,&hitb)-RAY_STEP; //margin since dist is not precise
  VMOV(step,*ray_dir);
  VMUL(step,RAY_STEP);
  VMOV(pad,*ray_dir);
  VMUL(pad,RAY_PAD);
  termf_t t;
  otree_t *RESTRICT tree = &vfx->tree;
  double dc = vfx->c;

  while (dist < end) {
    VPRC(vcnt++);
    uint32_t color = ot_get_termf(&t, tree, dc, p.x, p.y, p.z)&0xFFFFFF;
    if (color) {
      //VSET(*hit,(double)x,(double)y,(double)z); //this gives rough texture
      VMOV(*hit,p); //this gives smoother texture
      
      /*
    int x = (int)p.x;
    int y = (int)p.y;
    int z = (int)p.z;
      xyz_t center, spt, dv;
      VSET(center,(double)x+0.5,(double)y+0.5,(double)z+0.5);
      VMOV(dv,*src);
      VSUB(dv,center);
      VNORM(dv);
      VMUL(dv,1.0);
      VMOV(*hit,center);
      VADD(*hit,dv);*/

      return color;
    }

#if 0
    //this affects shading, and should not be used with a few voxels cubes
    //i.e. it must a an option, turned off for small scale models.
    if (t.c == 1) {
      VADD(p,step);
      dist += RAY_STEP;
      continue;
    }
#endif


    double sx = t.x;
    double sy = t.y;
    double sz = t.z;
    double c = t.c;
#if 0
    int GotHit = ray_cube_hit(&hita, &hitb, src, ray_dir
                  ,sx,sy,sz,sx+c,sy+c,sz+c);
    if (!GotHit) {
      VADD(p,step);
      dist += RAY_STEP;
      continue;
    }
#else
    //no need to check for return - it always happens here
    ray_cube_hit(&hita, &hitb, src, ray_dir, sx,sy,sz,sx+c,sy+c,sz+c);
#endif

    double dinc = vdist(&p,&hitb);
    if (dinc < 0.0001) {
      dist += RAY_STEP;
      VADD(p,step);
    } else {
      dist += dinc;
      VMOV(p,hitb);
      dist += RAY_PAD+EPS;
      VADD(p,pad);
    }
  }
  return 0;
}

#else
static INLINE
uint32_t raymarch(vfx_t *RESTRICT vfx
                 ,xyz_t *RESTRICT hit
                 ,xyz_t *RESTRICT src
                 ,xyz_t *RESTRICT ray_dir) {
  double dd = vfx->d;
  xyz_t hita, hitb, dir;
  int GotHit = ray_cube_hit
                      (&hita, &hitb, src, ray_dir
                      ,0.0,0.0,0.0,dd,dd,dd); //FIXME: vfx WxHxD should be used
  if (!GotHit) return 0;

  VPRC(rcnt++);

#if 0 //to debug the cube itself
  VMOV(*hit,hita);
  return 0x00FF00|OT_TERM;
#endif

  VMOV(dir,*ray_dir);
  int l = (int)vdist(&hita,&hitb)+1; //add 1, since dist is not precise

  if (dd < 16.0) {
    VMUL(dir,0.1);
    l *= 10;
  }
  term_t t;
  otree_t *RESTRICT tree = &vfx->tree;
  while (l) {
    VPRC(vcnt++);
    int x = (int)hita.x;
    int y = (int)hita.y;
    int z = (int)hita.z;
    //uint32_t color = vfx_get(vfx,x,y,z);
    uint32_t color = ot_get_term(&t, tree, vfx->c, x, y, z)&0xFFFFFF;

    if (!color) {
      l--;
      VADD(hita,dir);
      continue;
    }

    VMUL(dir, 0.1);
    while (1) { //walk back in smaller step, ensures we havent hopped a voxel
      VSUB(hita,dir);
      int tx = (int)hita.x;
      int ty = (int)hita.y;
      int tz = (int)hita.z;
      uint32_t tcolor = vfx_get(vfx,tx,ty,tz);
      if (!tcolor) break;
      color = tcolor;
      x = tx;
      y = ty;
      z = tz;
    }

    //VSET(*hit,(double)x,(double)y,(double)z); //this gives rough texture
    VMOV(*hit,hita); //this gives smoother texture

    return color;
  }
  return 0;
}
#endif



static INLINE
int ray_cube_hit(xyz_t *RESTRICT hita, xyz_t *RESTRICT hitb //hit points
                ,xyz_t *RESTRICT src, xyz_t *RESTRICT dir //ray origin/direction
                ,double sx, double sy, double sz  //min x,y,z
                ,double ex, double ey, double ez  //max x,y,z
          ) {
  xyz_t had, hbd;
  xyz_t *hp = hita; //current hit point
  xyz_t o_corner, far_corner;
  xyz_t  side_n,  top_n,  front_n; //face normals
  xyz_t nside_n, ntop_n, nfront_n; //negated face normals

  VSET(o_corner,sx,sy,sz);

  VSET(front_n , 0.0, 0.0, 1.0);
  ray_plane_hit(hp, src, dir, &o_corner, &front_n);
  if (!(hp->x < sx || hp->x >= ex || hp->y < sy || hp->y >= ey)) {
    hp = hitb;
  }

  VSET(side_n  , 1.0, 0.0, 0.0);
  ray_plane_hit(hp, src, dir, &o_corner, &side_n);
  if (!(hp->z < sz || hp->z >= ez || hp->y < sy || hp->y >= ey)) {
    if (hp == hitb) goto sort_hits;
    hp = hitb;
  }

  VSET(top_n   , 0.0, 1.0, 0.0);
  ray_plane_hit(hp, src, dir, &o_corner, &top_n);
  if (!(hp->x < sx || hp->x >= ex || hp->z < sz || hp->z >= ez)) {
    if (hp == hitb) goto sort_hits;
    hp = hitb;
  }

  VSET(far_corner,ex,ey,ez);

  VSET(nfront_n, 0.0, 0.0,-1.0);
  ray_plane_hit(hp, src, dir, &far_corner, &nfront_n);
  if (!(hp->x < sx || hp->x >= ex || hp->y < sy || hp->y >= ey)) {
    if (hp == hitb) goto sort_hits;
    hp = hitb;
  }

  VSET(nside_n ,-1.0, 0.0, 0.0);
  ray_plane_hit(hp, src, dir, &far_corner, &nside_n);
  if (!(hp->z < sz || hp->z >= ez || hp->y < sy || hp->y >= ey)) {
    if (hp == hitb) goto sort_hits;
    hp = hitb;
  }

  if (hp != hitb) return 0; //ray hitting cube always hits two faces

  VSET(ntop_n  , 0.0,-1.0, 0.0);
  ray_plane_hit(hp, src, dir, &far_corner, &ntop_n);
  //assume guaranteed hit
  /*if (!(hp->x < sx || hp->x >= ex || hp->z < sz || hp->z >= ez)) {
    if (hp == hitb) goto sort_hits;
    hp = hitb;
  }*/

  // should never happen with our setup
  //VMOV(*hitb,*hita); //single point hit
  //return 1;

sort_hits: //ensure hita is the first point on this ray hitting the cube
  VMOV(had,*hita);
  VMOV(hbd,*hitb);
  //with dir it should work without that
  //VSUB(had,*src);
  //VSUB(hbd,*src);
  if (VDOT(had,*dir) > VDOT(hbd,*dir)) { //compare lengths from src
    VSWAP(*hita,*hitb);
  }
  return 1;
}


static INLINE
int is_jailed_voxel(vfx_t *vfx, uint32_t x, uint32_t y, uint32_t z) {
  return vfx_get(vfx,x-1,y,z) && vfx_get(vfx,x+1,y,z)
      && vfx_get(vfx,x,y-1,z) && vfx_get(vfx,x,y+1,z)
      && vfx_get(vfx,x,y,z-1) && vfx_get(vfx,x,y,z+1);
}


//this one checks for nd==0.0
static INLINE
int ray_plane_hit(xyz_t *RESTRICT hit
                 ,xyz_t *RESTRICT src, xyz_t *RESTRICT dir
                 ,xyz_t *RESTRICT plane_coord, xyz_t *RESTRICT plane_normal) {
  double nd = VDOT(*plane_normal,*dir);
  if (nd==0.0) return 0; //No intersection, the line is parallel to the plane
  double d = VDOT(*plane_normal,*plane_coord);
  double l = (d -  VDOT(*plane_normal,*src))/nd; //length until hit
  VMOV(*hit,*dir);
  VMUL(*hit,l);
  VADD(*hit,*src);
  return 1;
}



//cube normals
static xyz_t side_n;
static xyz_t top_n;
static xyz_t front_n;
static xyz_t nside_n;
static xyz_t ntop_n;
static xyz_t nfront_n;

static INLINE
void init_cube_normals(vfx_t *vfx) {
  VSET(side_n  , 1.0, 0.0, 0.0);
  VSET(top_n   , 0.0, 1.0, 0.0);
  VSET(front_n , 0.0, 0.0, 1.0);
  VSET(nside_n ,-1.0, 0.0, 0.0);
  VSET(ntop_n  , 0.0,-1.0, 0.0);
  VSET(nfront_n, 0.0, 0.0,-1.0);
}


//this one uses older ray_plane_hit, requiring a check
static INLINE
int ray_cube_hit(xyz_t *RESTRICT hita, xyz_t *RESTRICT hitb
                ,xyz_t *RESTRICT src, xyz_t *RESTRICT dir
                ,double sx, double sy, double sz
                ,double ex, double ey, double ez 
          ) {
  xyz_t had, hbd;
  xyz_t *hp = hita; //current hit point
  xyz_t o_corner, far_corner;

  VSET(o_corner,sx,sy,sz);
  if (ray_plane_hit(hp, src, dir, &o_corner, &front_n)) {
    if (!(hp->x < sx || hp->x >= ex || hp->y < sy || hp->y >= ey)) {
      hp = hitb;
    }
  }

  if (ray_plane_hit(hp, src, dir, &o_corner, &side_n)) {
    if (!(hp->z < sz || hp->z >= ez || hp->y < sy || hp->y >= ey)) {
      if (hp == hitb) goto sort_hits;
      hp = hitb;
    }
  }

  if (ray_plane_hit(hp, src, dir, &o_corner, &top_n)) {
    if (!(hp->x < sx || hp->x >= ex || hp->z < sz || hp->z >= ez)) {
      if (hp == hitb) goto sort_hits;
      hp = hitb;
    }
  }

  VSET(far_corner,ex,ey,ez);
  if (ray_plane_hit(hp, src, dir, &far_corner, &nfront_n)) {
    if (!(hp->x < sx || hp->x >= ex || hp->y < sy || hp->y >= ey)) {
      if (hp == hitb) goto sort_hits;
      hp = hitb;
    }
  }

  if (ray_plane_hit(hp, src, dir, &far_corner, &nside_n)) {
    if (!(hp->z < sz || hp->z >= ez || hp->y < sy || hp->y >= ey)) {
      if (hp == hitb) goto sort_hits;
      hp = hitb;
    }
  }

  if (hp != hitb) return 0;

  if (ray_plane_hit(hp, src, dir, &far_corner, &ntop_n)) {
    if (!(hp->x < sx || hp->x >= ex || hp->z < sz || hp->z >= ez)) {
      if (hp == hitb) goto sort_hits;
      hp = hitb;
    }
  }

  //fprintf(stderr, "hello\n");
  // should never happen with our setup
  //VMOV(*hitb,*hita); //single point hit
  //return 1;

sort_hits: //ensure hita is the first point on ray to hitting the cube
  VMOV(had,*hita);
  VMOV(hbd,*hitb);
  //with dir it should work without that
  //VSUB(had,*src);
  //VSUB(hbd,*src);
  if (VDOT(had,*dir) > VDOT(hbd,*dir)) { //compare lengths from src
    VSWAP(*hita,*hitb);
  }
  return 1;
}





#define VGET(vfx,x,y,z) (ot_get(&vfx->tree,0,vfx->c,(x),(y),(z))&0x00FFFFFF)
#define VGETU(vfx,x,y,z) ot_get(&vfx->tree,0,vfx->c,(x),(y),(z))

// sample cube face
void vfx_sample_face(vfx_t *vfx, gfx_t *cbuf, gfx_t *zbuf, int face) {
  int x,y,z;
  uint32_t *colors = cbuf->data;
  uint32_t *depths = zbuf->data;
  int cw = cbuf->w;
  int ch = cbuf->h;
  uint32_t dim = vfx->d;
  //double max_dist = (double)(dim*2 + 10);
  double max_dist = sqrt((double)(dim*dim*3))*2.0+0.00001;
  switch (face) {
  case F_FRONT:
    for (y = 0; y < dim; y++) {
      for (x = 0; x < dim; x++) {
        colors[cw*y + x] = 0xFF000000;
        depths[cw*y + x] = 0;
        for (z = 0; z < dim; z++) {
          uint32_t v = VGET(vfx,x,y,z);
          if (v) {
            double dist = z;
            double b = max_dist - dist;
            if (b < 0.0) b = 0.0;
            uint32_t c = (uint32_t)(b*255.0/max_dist);
            colors[cw*y + x] = v;
            depths[cw*y + x] = c;
            break;
          }
        }
      }
    }
    break;
  case F_BACK:
    for (y = 0; y < dim; y++) {
      for (x = 0; x < dim; x++) {
        int rx = cw-1-x;
        colors[cw*y + rx] = 0xFF000000;
        depths[cw*y + rx] = 0;
        for (z = dim-1; z >= 0; z--) {
          uint32_t v = VGET(vfx,x,y,z);
          if (v) {
            double dist = dim-1-z;
            double b = max_dist - dist;
            if (b < 0.0) b = 0.0;
            uint32_t c = (uint32_t)(b*255.0/max_dist);
            colors[cw*y + rx] = v;
            depths[cw*y + rx] = c;
            break;
          }
        }
      }
    }
    break;
  case F_LEFT:
    for (y = 0; y < dim; y++) {
      for (z = 0; z < dim; z++) {
        int rz = cw-1-z;
        colors[cw*y + rz] = 0xFF000000;
        depths[cw*y + rz] = 0;
        for (x = 0; x < dim; x++) {
          uint32_t v = VGET(vfx,x,y,z);
          if (v) {
            double dist = x;
            double b = max_dist - dist;
            if (b < 0.0) b = 0.0;
            uint32_t c = (uint32_t)(b*255.0/max_dist);
            colors[cw*y + rz] = v;
            depths[cw*y + rz] = c;
            break;
          }
        }
      }
    }
    break;
  case F_RIGHT:
    for (y = 0; y < dim; y++) {
      for (z = 0; z < dim; z++) {
        colors[cw*y + z] = 0xFF000000;
        depths[cw*y + z] = 0;
        for (x = dim-1; x >= 0; x--) {
          uint32_t v = VGET(vfx,x,y,z);
          if (v) {
            double dist = dim-1-x;
            double b = max_dist - dist;
            if (b < 0.0) b = 0.0;
            uint32_t c = (uint32_t)(b*255.0/max_dist);
            colors[cw*y + z] = v;
            depths[cw*y + z] = c;
            break;
          }
        }
      }
    }
    break;
  case F_TOP:
    for (x = 0; x < dim; x++) {
      for (z = 0; z < dim; z++) {
        int rz = dim-1-z;
        colors[cw*rz + x] = 0xFF000000;
        depths[cw*rz + x] = 0;
        for (y = 0; y < dim; y++) {
          uint32_t v = VGET(vfx,x,y,z);
          if (v) {
            double dist = y;
            double b = max_dist - dist;
            if (b < 0.0) b = 0.0;
            uint32_t c = (uint32_t)(b*255.0/max_dist);
            colors[cw*rz + x] = v;
            depths[cw*rz + x] = c;
            break;
          }
        }
      }
    }
    break;
  case F_BOTTOM:
    for (x = 0; x < dim; x++) {
      for (z = 0; z < dim; z++) {
        colors[cw*z + x] = 0xFF000000;
        depths[cw*z + x] = 0;
        for (y = dim-1; y >= 0; y--) {
          uint32_t v = VGET(vfx,x,y,z);
          if (v) {
            double dist = dim-1-y;
            double b = max_dist - dist;
            if (b < 0.0) b = 0.0;
            uint32_t c = (uint32_t)(b*255.0/max_dist);
            colors[cw*z + x] = v;
            depths[cw*z + x] = c;
            break;
          }
        }
      }
    }
    break;
  default:
    fprintf(stderr, "vfx_sample_face: invalid face=%d\n", face);
    error_halt(0);
    break;
  }
}
