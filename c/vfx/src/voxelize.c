#include <ncu/util.h>

#:slb
#:mesh

#cubefaces(sa,ea) {
//used to iterate over voxels on the cube faces
do {
int x,y,z;
//front
for (x = sx sa; x <= ex ea; x++) {
  for (y = sy sa; y <= ey ea; y++) {
    //for (z = sz; z <= ez; z++) {
    process_oct(y,x,y,sz);
    //}
  }
}
//back
for (x = sx sa; x <= ex ea; x++) {
  for (y = sy sa; y <= ey ea; y++) {
    //for (z = sz; z <= ez; z++) {
    process_oct(y,x,y,ez);
    //}
  }
}
//top
for (x = sx sa; x <= ex ea; x++) {
  //for (y = sy; y <= ey; y++) {
  for (z = sz sa; z <= ez ea; z++) {
    process_oct(z,x,sy,z);
  }
  //}
}
//bottom
for (x = sx sa; x <= ex ea; x++) {
  //for (y = sy; y <= ey; y++) {
  for (z = sz sa; z <= ez ea; z++) {
    process_oct(z,x,ey,z);
  }
  //}
}

//right
//for (x = sx; x <= ex; x++) {
  for (y = sy sa; y <= ey ea; y++) {
    for (z = sz sa; z <= ez ea; z++) {
      process_oct(z,sx,y,z);
    }
  }
//}

//left
//for (x = sx; x <= ex; x++) {
  for (y = sy sa; y <= ey ea; y++) {
    for (z = sz sa; z <= ez ea; z++) {
      process_oct(z,ex,y,z);
    }
  }
//}
} while(0)
}




TLst(OctantList,octant_t)

void vxFillInterior(slab_t *slb) {
  int x, y, z;
  int w = slb->w, h = slb->h, d = slb->d;
  auto svo = slb.svo;
  segs_t *segs = svo.list_segs;
  U8 **segt = malloc(segs.len*sizeof(void*));
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    segt[j] = malloc(seg.maxid);
    memset(segt[j], 0, seg.maxid);
  }

#if 0
#process_oct(iter,ix,iy,iz) {do {
    svo.set(ix,iy,iz)->color = 0x7f7f7f;
  } while (0)}
#elif 0
#process_oct(iter,ix,iy,iz) {do {
    printf("%d,%d,%d\n", (ix)-1,(iy)-1,(iz)-1);
  } while (0)}
#else
#process_oct(iter,ix,iy,iz) {do {
    octant_t o;
    if ((U32)(ix) >= (U32)w || (U32)(iy) >= (U32)h || (U32)(iz) >= (U32)d) {
      continue;
    }
    attr_t *attr = svo.get_oct(ix,iy,iz,&o);
    if (attr.is_empty && !segt[o.segid][o.octid]) {
      segt[o.segid][o.octid] = 1;
      octl.push(o);
      /* say(o.x,",",o.y,",",o.z,",",(S32)o.c,",",o.octid); */
    }
   iter += o.c - 1; //FIXME: should it match cubeface ea arg?
  } while (0)}
#endif

  OctantList octl;
  int sx = 0, sy = 0, sz = 0;
  int ex = w-1, ey = h-1, ez = d-1;

  //include all empty edge voxels
  cubefaces(+0,-0);

  while (octl.len) {
    octant_t o = octl.pop;
    int sx = (S32)o.x-1, sy = (S32)o.y-1, sz = (S32)o.z-1;
    int ex = sx+o.c+1, ey = sy+o.c+1, ez = sz+o.c+1;
    //FIXME: better solution for out of bounds clamp
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sz < 0) sz = 0;
    if (ex >= w) ex = w-1;
    if (ey >= h) ey = h-1;
    if (ez >= d) ez = d-1;
    // include all neiboring empty voxels
    cubefaces(+1,-1);
  }
  octl.dtor;

  U32 clay_color = slb->svo.clay_color;

  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    SVOTerms *terms = seg.empty_terms;
    U32 segid = seg.id;
    U32 topoct = seg->topo.len<<3;
    int nmods = 0;
    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      int x = t.x;
      int y = t.y;
      int z = t.z;
      if (XYZ_OUTSIDE(x,y,z,slb->w,slb->h,slb->d)) continue;
      U32 octid = seg.term_octid(t);
      if (octid < topoct) {
        if (segt[segid][octid]) continue;
      }
      if (t->c != 1) {
        nmods++;
        seg.term_split(t,terms);
        continue;
      }
      attr_t *attr = seg.term_attr(t);
      attr->color = clay_color;
      attr->normal = CLAY_NORMAL;
      seg.term_set(t, clay_color);
    }
    delete(terms);
    if (nmods) seg.compact;
  }

  for (int j = 0; j < segs.len; j++) free(segt[j]);
  free(segt);
  delete(segs);
}

static gfx_t *diffuse;

void vxSetImportTexture(int type, gfx_t *gfx) {
  if (type == 0) diffuse = gfx;
}

inline F32 vec2.dot(vec2 *v) {
  return this.x*v.x + this.y*v.y;
}

inline vec2 vec2.n {
  return (*this) / sqrtf(this.dot(this));
}

void vxVoxelize(slab_t *slb, CMesh *mesh) {
  auto vertices = &mesh->vertices;
  auto faces = &mesh->faces;
  
  gfx_t *td = diffuse;
  diffuse = 0;

  //larger voxelization can be done by
  //1. split the large cube into sub cubes
  //2. for each cube picking only the tirangles intersecting it
  //3. rendering these triangles
  //4. incorporating the cube into SVO
  int w = slb->w;
  int h = slb->h;
  int d = slb->d;
  U32 uscale = MIN(MIN(w,h),d);
  float scale = uscale;
  U32 cube_size = scale*scale*scale;
  U32 *cube = malloc(cube_size*sizeof(U32));
  memset4(cube, NIL_COLOR, cube_size);

  F32 step = 0.25/scale;

  U32 dw = 0;
  U32 dh = 0;
  U32 *dp = 0;

  if (td) {
    dw = td->w;
    dh = td->h;
    dp = td->data;
  }

  U32 clay_color = slb->svo.clay_color;

  for (int i = 0; i < faces.len; i++) {
    CFace *f = &faces.elts[i];
    CVertex a = vertices.elts[f->vs[0]];
    CVertex b = vertices.elts[f->vs[1]];
    CVertex c = vertices.elts[f->vs[2]];
    vec3 bad = b.xyz - a.xyz;
    vec3 cad = c.xyz - a.xyz;
    vec3 cbd = c.xyz - b.xyz;
    F32 badl = vlen(bad);
    F32 cadl = vlen(cad);
    F32 cbdl = vlen(cbd);

    F32 l;
    CVertex s,e,o;
    if (cbdl > badl && cbdl > cadl) {
      l = cbdl;
      s = b;
      e = c;
      o = a;
    } else if (badl > cbdl && badl > cadl) {
      l = badl;
      s = a;
      e = b;
      o = c;
    } else { //else cadl is the longest edge
      l = cadl;
      s = a;
      e = c;
      o = b;
    }
    if (fabs(l) <= EPS) continue; //FIXME: set single voxel s
    vec3 p = s.xyz;
    vec3 esd = e.xyz - p;
    vec3 sstep = normalized(esd)*step;
    vec2 puv = s.uv;
    vec2 uvsd = e.uv - puv;
    vec2 uvstep = uvsd.n*step;
    for ( ; l > 0.0f; l -= step) {
      vec3 t = p;
      vec3 otd = o.xyz - t;
      F32 k = vlen(otd);
      vec3 tstep = normalized(otd)*step;

      vec2 tuv = puv;
      vec2 uvotd = o.uv - tuv;
      vec2 tuvstep = uvotd.n * step;

      for ( ; k > 0.0f; k -= step) {
        U32 vx = (w-1)*t.x; //FIXME: premultiply vstep and s
        U32 vy = (h-1)*t.y;
        U32 vz = (d-1)*t.z;
        if (vx < uscale && vy < uscale && vz < uscale) {
          //FIXME: exact triangle-cube intersect code can still be used
          //       to improve precission, if we also overdraw a bit: k>=0.0f

          U32 dtexel = clay_color;
          if (dp) {
            F32 u = tuv.x;
            F32 v = 1.0f - tuv.y;
            U32 tx = u*(dw-1);
            U32 ty = v*(dh-1);
            if (tx >= dw) tx = dw-1;
            if (ty >= dh) ty = dh-1;
            dtexel = dp[ty*dw + tx];
          }
          cube[vz*uscale*uscale + vy*uscale + vx] = dtexel;
        }
        t += tstep;
        tuv += tuvstep;
      }
      p += sstep;
      puv += uvstep;
    }
  }

  vxClear(slb, NIL_COLOR);
  vxSetClayColor(slb, clay_color);

  U32 softness = gsoftness<<24;

  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    if (!slb.vcontains(seg.xyz)) continue;
    SVOTerms *terms = seg.terms;

    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      U32 sx = t.x;
      U32 sy = t.y;
      U32 sz = t.z;
      U32 c = t->c;

      if (c != 1) {
        seg.term_split(t,terms);
        continue;
      }
      
      if (sx >= uscale || sy >= uscale || sz >= uscale) continue;
  
      U32 voxel = cube[sz*uscale*uscale + sy*uscale + sx];
      if (voxel == NIL_COLOR) continue;

      attr_t *attr = seg.term_attr(t);
      attr->color = voxel|softness;
      attr->normal = CLAY_NORMAL;
    }
    delete(terms);
    seg.compact;
  }
  delete(segs);
}
