//FIXME: move this to gfx
#KD_DIM 2
#:kdtree


TLst(KDNodes,KDNode)

inline KDNode mk_kdnode2(float x, float y) {
  KDNode n;
  n.x[0] = x;
  n.x[1] = y;
  n.left = 0;
  n.right = 0;
  return n;
}

inline vec3 kd_vnearest(KDNode *root, vec3 point) {
  KDNode p;
  p.x[0] = point.x;
  p.x[1] = point.y;
  KDNode *r = kd_nearest(root, &p);
  if (!r) return point;
  return (vec3){r->x[0],r->x[1],0.0f};
}



static float *gfx_radial_field(gfx_t *hrz, float *af) {
  U32 *phrz = hrz->data;

  U32 hw = hrz->w;
  U32 hh = hrz->h;

  KDNodes outline; //outline points

#HE(x,y) (phrz[(y)*hw + (x)]&0xFF000000)
  int x,y;
  for (y = 0; y < hh; y++) {
    for (x = 0; x < hw; x++) {
      if (HE(x,y)) continue;
      if (x == 0 || x == hw-1 || y == 0 || y == hh-1) {
        outline.push(mk_kdnode2(x,y));
        continue;
      }
      if (HE(x+1,y) || HE(x-1,y) || HE(x,y+1) || HE(x,y-1)) {
        outline.push(mk_kdnode2(x,y));
        continue;
      }
#if 0
      if (HE(x+1,y+1) || HE(x-1,y-1) || HE(x+1,y-1) || HE(x-1,y+1)) {
        outline.push(mk_kdnode2(x,y));
        continue;
      }
#endif
    }
  }

  //center of mass x,y
  double cmx = 0.0;
  double cmy = 0.0;
  for (int i = 0; i < outline.len; i++) {
    KDNode *n = outline.ref(i);
    cmx += n->x[0];
    cmy += n->x[1];
  }
  cmx /= outline.len;
  cmy /= outline.len;

  //fprintf(stdout, "%d\n", outline.len);
  KDNode *kdroot = kd_make(outline.elts, outline.len);

  U32 sdf_size = hw*hh*sizeof(float);
  float *sdf = malloc(sdf_size);
  float *rf = malloc(sdf_size);
  memset(sdf, 0, sdf_size);
  memset(rf, 0, sdf_size);
  for (y = 0; y < hh; y++) {
    for (x = 0; x < hw; x++) {
      if (HE(x,y)) continue;
      vec3 point = (vec3){x,y,0};
      vec3 mesh_point = kd_vnearest(kdroot, point);
      sdf[y*hw + x] = distance(point,mesh_point);
    }
  }


  for (y = 0; y < hh; y++) {
    for (x = 0; x < hw; x++) {
      if (HE(x,y)) continue;
      U32 hx = x;
      U32 hy = y;
#XY(x,y) sdf[(y)*hw + (x)]
      float cdist = XY(hx,hy);
#TRY_XY(x,y) \
      { \
        U32 tx = (x); \
        U32 ty = (y); \
        if (tx<(U32)hrz->w && ty<(U32)hrz->h) { \
          float tsi = XY(tx,ty); \
          if (tsi > cdist) { \
            cdist = tsi; \
            ncx = tx; \
            ncy = ty; \
          } \
        } \
      }
      U32 ncx=hx, ncy=hy;
      U32 cx, cy;
      do {
        cx = ncx;
        cy = ncy;
        TRY_XY(cx+1,cy);
        TRY_XY(cx-1,cy);
        TRY_XY(cx,cy+1);
        TRY_XY(cx,cy-1);
#if 1
        TRY_XY(cx+1,cy+1);
        TRY_XY(cx-1,cy-1);
        TRY_XY(cx-1,cy+1);
        TRY_XY(cx+1,cy-1);
#endif
      } while (ncx != cx || ncy != cy);
      //if (cx != 24 || cy != 24) fprintf(stdout, "%d,%d\n", cx,cy);
      float dist;
      float angle = -1.0f;
      if (cx == hx && cy == hy) dist = 0.0;
      else {
        float hdist = distance((vec3){cx,cy,0},(vec3){hx,hy,0});
        float dx = ((float)hx-cx)/hdist;
        float dy = ((float)hy-cy)/hdist;
        float step = 0.25f;
        dx *= step;
        dy *= step;
        float fhx = hx;
        float fhy = hy;
        U32 thx = hx;
        U32 thy = hy;
        for (;;) {
          U32 hx = roundf(fhx);
          U32 hy = roundf(fhy);
          if (hx>=(U32)hrz->w || hy>=(U32)hrz->h) break;
          uint32_t pixel = phrz[hy*hrz->w+hx];
          uint32_t transparent = pixel&0xFF000000;
          if (transparent) break;
          thx = hx;
          thy = hy;
          fhx += dx;
          fhy += dy;
        }
        float cmdist = distance((vec3){cmx,cmy,0},(vec3){hx,hy,0});
        float cmdx = ((float)hx-cmx)/cmdist;
        float cmdy = ((float)hy-cmy)/cmdist;
        angle = atan2(cmdy,cmdx)/(PI*2) + 0.5;
        //fprintf(stdout, "%f,%f,%f\n", angle, ddx,ddy);
        float tdist = distance((vec3){cx,cy,0},(vec3){thx,thy,0});
        if (tdist < 0.001) dist = 0.0;
        else {
          dist = hdist / tdist;
          if (dist > 1.0f) dist = 1.0f; //should never happen
        }
      }
#undef TRY_XY
#undef XY
      int index = hy*hw + hx;
      rf[index] = dist;
      if (af) af[index] = angle;
    }
  }
#undef HE

  outline.free;
  free(sdf);
  return rf;
}
