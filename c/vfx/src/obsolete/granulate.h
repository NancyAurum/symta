void vxNormalizeBox(slab_t *slb) {
  vxGranulate(slb);
  cut_box(slb, 1, 0,0,0,slb->w,slb->h,slb->d);
}


//calc normals for all the voxels on the box faces
static void vxGranulateBox(slab_t *slb
   ,int32_t sx, int32_t sy, int32_t sz
   ,int32_t w, int32_t h, int32_t d) {
  int32_t x,y,z;
  auto tree = slb.svo;
  int32_t ex=sx+w, ey=sy+h, ez=sz+d;

  if (sx < 0) sx = 0;
  if (sy < 0) sy = 0;
  if (sz < 0) sz = 0;
  /*if (ex >= slb->w) ex = slb->w-1;
  if (ey >= slb->h) ey = slb->h-1;
  if (ez >= slb->d) ez = slb->d-1;*/

  //FIXME: the following can be optimized, since the goal is only to split terms
  //       into fine gained mesh
#define REMESH_BODY \
  { \
    attr_t *v = tree.get(x,y,z); \
    if (v.is_empty) continue; \
    attr_t *attr = tree.set(x,y,z); \
    attr->normal = MESH_NORMAL; \
  }

  x = sx;
  //for (x = sx; x < ex; x++)
  for (y = sy; y < ey; y++)
  for (z = sz; z < ez; z++)
    REMESH_BODY

  y = sy;
  for (x = sx; x < ex; x++)
  //for (y = sy; y < ey; y++)
  for (z = sz; z < ez; z++)
    REMESH_BODY

  z = sz;
  for (x = sx; x < ex; x++)
  for (y = sy; y < ey; y++)
  //for (z = sz; z < ez; z++)
    REMESH_BODY

  x = ex-1;
  //for (x = sx; x < ex; x++)
  for (y = sy; y < ey; y++)
  for (z = sz; z < ez; z++)
    REMESH_BODY

  y = ey-1;
  for (x = sx; x < ex; x++)
  //for (y = sy; y < ey; y++)
  for (z = sz; z < ez; z++)
    REMESH_BODY

  z = ez-1;
  for (x = sx; x < ex; x++)
  for (y = sy; y < ey; y++)
  //for (z = sz; z < ez; z++)
    REMESH_BODY
}


void vxGranulate(slab_t *slb) {
  auto tree = slb.svo;
  vxGranulateBox(slb,0,0,0,slb->w,slb->h,slb->d);
  SVOTerms *terms = vxListTerms(slb);
  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    if (tree.exists(t)) continue; //we care only about empty terms
    int32_t sx = t->o.x;
    int32_t sy = t->o.y;
    int32_t sz = t->o.z;
    int32_t cc = t->c+2;
    vxGranulateBox(slb,sx-1,sy-1,sz-1,cc,cc,cc);
  }
  delete(terms);
}
