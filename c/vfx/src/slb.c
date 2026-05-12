/* Voxel Graphics Library
*/


#:slb
#:gfx_primitives


camera_t gcam;


//for vxDbg
int dbg = 0;

int bug = 0;

static int error_halt(int n) {
  return 1/n;
}

U32 vxW(slab_t *slb) { return slb->w; }
U32 vxH(slab_t *slb) { return slb->h; }
U32 vxD(slab_t *slb) { return slb->d; }

static void vxPrintTerms(slab_t *slb) {
  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    int segid = seg.id;
    SVOTerms *terms = seg.terms;
    fprintf(stderr,"terms: %d:%d/%d\n", segid, terms.len, terms.total_size);
    int i;
    for (i = 0; i < terms.len; i++) {
      auto t = terms.ref(i);
      fprintf(stderr, "%d: %d,%d,%d,%c\n", i, t.x, t.y, t.z, t->c);
    }
    delete(terms);
  }
  delete(segs);
}

void vxCompact(slab_t *slb) {
  slb.svo.compact;
}

//clear interior of the mesh
void vxClearInterior(slab_t *slb, U32 clear_color) {
  slb.svo.clear_interior(clear_color);
  vxCompact(slb);
}

U32 vxGetClayColor(slab_t *slb) {
  return slb.svo.clay_color;
}

void vxSetClayColor(slab_t *slb, U32 color) {
  slb.svo.clay_color = color;
}

void vxClear(slab_t *slb, U32 color) {
  slb.svo.clear(color);
  if (color != NIL_COLOR) vxCutOutlierTerms(slb);
}

static U32 whd2scale(U32 w, U32 h, U32 d) {
  U32 c = 1;
  U32 g = (MAX(MAX(w,h),d)+1)/2; //align to neares power of 2
  U32 scale = 0;
  while (c < g) {
    c *= 2;
    scale++;
  }
  return scale;
}

static void init_slb(slab_t *slb, U32 w, U32 h, U32 d) {
  slb->w = w;
  slb->h = h;
  slb->d = d;
  slb->box = (vec3){w+EPS,h+EPS,d+EPS};
  slb->flags = 0;
  slb->info = 0;
  U32 scale = whd2scale(w,h,d);
  slb.svo.init(slb, scale);
  slb->nil = 0;
  slb->abyss = 0;
}

slab_t *vxNew(U32 w, U32 h, U32 d) {
  slab_t *slb = (slab_t*)malloc(sizeof(slab_t));
  if (!slb) return 0;
  init_slb(slb, w, h, d);
  return slb;
}

void vxFree(slab_t *slb) {
  slb.svo.free;
  if (slb->info) free(slb->info);
  free(slb);
}

slab_t *vxClone(slab_t *slb) {
  slab_t *c = vxNew(slb->w,slb->h,slb->d);
  c.svo.clone(slb.svo);
  c->xyz = slb->xyz;
  c->center = slb->center;
  c->angle = slb->angle;
  c->scale = slb->scale;
  return c;
}

char *vxInfo(slab_t *slb) {
  return slb->info ? slb->info : "";
}

void vxSetInfo(slab_t *slb, char *info) {
  if (slb->info) free(slb->info);
  slb->info = strdup(info);
}

U32 vxGet(slab_t *slb, int x, int y, int z) {
  if (XYZ_OUTSIDE(x,y,z,slb->w,slb->h,slb->d)) return NIL_COLOR;
  attr_t *attr = slb.svo.get(x, y, z);
  if (attr.is_empty) return NIL_COLOR;
  return attr->color&0xFFFFFF; //mask out the normal softness
}

U32 vxGetTr(slab_t *slb, int x, int y, int z) {
  if (XYZ_OUTSIDE(x,y,z,slb->w,slb->h,slb->d)) return slb->abyss;
  attr_t *attr = slb.svo.get(x, y, z);
  if (attr.is_empty) return slb->nil;
  return attr->color&0xFFFFFF; //mask out the normal softness
}

void vxSetNilAbyss(slab_t *slb, U32 nil, U32 abyss) {
  slb->nil = nil;
  slb->abyss = abyss;
}


inline void invalidateNeibNormals(slab_t *slb, int cx, int cy, int cz) {
  float w = slb->w;
  float h = slb->h;
  float d = slb->d;
  ivec3 p = {cx,cy,cz};
  for (int i = 0; i < DIRS3D_COUNT; i++) {
    ivec3 q = p+idirs3d[i];
    if (XYZ_OUTSIDE(q.x,q.y,q.z,w,h,d)) continue;
    attr_t *attr = slb.svo.get(q.x,q.y,q.z);
    if (attr->normal > MESH_NORMAL) continue;
    attr->normal = CLAY_NORMAL;
  }
}

//FIXME: vxSet clear normals of the neigboring voxels.
void vxSet(slab_t *slb, int x, int y, int z, U32 color) {
  if (XYZ_OUTSIDE(x,y,z,slb->w,slb->h,slb->d)) return;
  if (color == NIL_COLOR) {
    slb.svo.erase(x,y,z);
  } else {
    attr_t *attr = slb.svo.set(x,y,z);
    attr->color = color;
    attr->normal = MESH_NORMAL;
    invalidateNeibNormals(slb, x, y, z);
  }
  //if (slb->svo.nmods > slb->svo.topo.len/2) vxCompact(slb);
  if (slb->svo.nmods > 32768) vxCompact(slb);
}

void vxSetTr(slab_t *slb, int x, int y, int z, U32 color) {
  if (color == slb->nil) color = NIL_COLOR;
  vxSet(slb, x, y, z, color);
}


void vxSetI(slab_t *slb, U32 index, U32 color) {
  int x = index/slb->d%slb->w;
  int y = index/(slb->w*slb->d);
  int z = index%slb->d;
  vxSetTr(slb, x, y, z, color);
}

U32 vxGetI(slab_t *slb, U32 index) {
  int x = index/slb->d%slb->w;
  int y = index/(slb->w*slb->d);
  int z = index%slb->d;
  return vxGetTr(slb, x, y, z);
}


//FIXME: vxSet clear normals of the neigboring voxels.
void vxSetRaw(slab_t *slb, int x, int y, int z, U32 color) {
  if (color == NIL_COLOR) {
    slb.svo.erase(x,y,z);
  } else {
    attr_t *attr = slb.svo.set(x,y,z);
    attr->color = color;
    attr->normal = MESH_NORMAL;
    invalidateNeibNormals(slb, x, y, z);
  }
}

int vxDbg(slab_t *slb, int new_dbg_state) {
  int prev = dbg;
  dbg = new_dbg_state;
  return prev;
}
