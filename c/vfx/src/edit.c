/* Voxel Graphics Library
*/


#:slb
#:gfx_primitives
#:edit

static void cut_box(slab_t *slb, int invert
                  , U32 bx, U32 by, U32 bz
                  , U32 bw, U32 bh, U32 bd) {
  U32 x,y,z;
  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    SVOTerms *terms = seg.filled_terms;
    int nmods;
    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      U32 c = t->c-1;
      U32 x = t.x-bx+c;
      U32 y = t.y-by+c;
      U32 z = t.z-bz+c;
      if (!XYZ_OUTSIDE(x,y,z,bw,bh,bd)) continue;
      if (t->c != 1) {
        nmods++;
        seg.term_split(t,terms);
        continue;
      }
      seg.term_erase(t);
    }
    delete(terms);
    if (nmods) seg.compact;
  }
  delete(segs);
}

void vxCutOutlierTerms(slab_t *slb) {
  cut_box(slb, 1, 0,0,0,slb->w,slb->h,slb->d);
}

slab_t *vxCopyBox(slab_t *slb, int x0, int y0, int z0, int x1, int y1, int z1) {
  U32 w = x1-x0;
  U32 h = y1-y0;
  U32 d = z1-z0;
  slab_t *c = vxNew(w,h,d);
  vxClear(c, NIL_COLOR);
  c.svo.clay_color = slb->svo.clay_color;
  segs_t *segs = c.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    if (!slb.vcontains(seg.xyz)) continue;
    SVOTerms *terms = seg.terms;
    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      int sx = t.x;
      int sy = t.y;
      int sz = t.z;
      int c = t->c;
      int x,y,z;
      for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
        for (x = sx; x < sx+c; x++) {
          if ((U32)x >= w && (U32)y >= h && (U32)z >= d) continue;
          int xx=x+x0;
          int yy=y+y0;
          int zz=z+z0;
          if (XYZ_OUTSIDE(xx,yy,zz,slb->w,slb->h,slb->d)) continue;
          attr_t *attr = slb.svo.get(xx, yy, zz);
          if (attr.is_empty) continue;
          if (c==1) {
            seg.term_set(t,attr->color);
          } else {
            seg.term_split(t,terms);
          }
          goto next_term;
        }
      }
      next_term:;
    }
    delete(terms);
    seg.compact;
  }
  delete(segs);
  return c;
}


void vxScaled(slab_t *slb, slab_t *src) {
  vxClear(slb, NIL_COLOR);
  slb.svo.clay_color = src.svo.clay_color;
  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    if (!slb.vcontains(seg.xyz)) continue;
    SVOTerms *terms = seg.terms;
    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      int sx = t.x;
      int sy = t.y;
      int sz = t.z;
      int c = t->c;
      int x,y,z;
      for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
        for (x = sx; x < sx+c; x++) {
          if (XYZ_OUTSIDE(x,y,z,slb->w,slb->h,slb->d))
            continue; //happens, since SVO has power of two WxHxD
          int xx = x*src->w/slb->w;
          int yy = y*src->h/slb->h;
          int zz = z*src->d/slb->d;
          attr_t *attr = src.svo.get(xx, yy, zz);
          if (attr.is_empty) continue;
          if (c==1) {
            seg.term_set(t,attr->color);
          } else {
            seg.term_split(t,terms);
          }
          goto next_term;
        }
      }
      next_term:;
    }
    delete(terms);
    seg.compact;
  }
  delete(segs);
}



char *vxMargins(slab_t *slb) {
  vec3 bmin = (vec3){INFINITY,INFINITY,INFINITY};
  vec3 bmax = (vec3){-INFINITY,-INFINITY,-INFINITY};
  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    SVOTerms *terms = seg.filled_terms;
    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      vec3 omin = (vec3){t.x, t.y, t.z};
      U32 c = t->c;
      vec3 omax = (vec3){t.x+c, t.y+c, t.z+c};
      bmin = vmin(bmin, omin);
      bmax = vmax(bmax, omax);
    }
    delete(terms);
  }
  delete(segs);
  sprintf(slb_sample_txt, "%d %d %d %d %d %d"
         ,(int)roundf(bmin.x), (int)roundf(bmin.y), (int)roundf(bmin.z)
         ,(int)roundf(bmax.x), (int)roundf(bmax.y), (int)roundf(bmax.z));

  return slb_sample_txt;
}


void vxSmoothNormals(slab_t *slb, U32 amount) {
  if (amount > 0xF) amount = amount;
  amount <<= 24;
  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    if (!slb.vcontains(seg.xyz)) continue;
    SVOTerms *terms = seg.filled_terms;

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

      attr_t *attr = seg.term_attr(t);
      if (attr.is_mesh) {
mesh:
        attr->color = (attr->color&0xFFFFFF) | amount;
        attr->normal = MESH_NORMAL;
        continue;
      }

      if (XYZ_BORDER_OR_OUTSIDE(sx,sy,sz,slb->w,slb->h,slb->d)) goto mesh;

      for (int k = 0; k < 6; k++) {
        ivec3 d = idirs3d6[k];
        attr_t *nattr = slb.svo.get(sx+d.x,sy+d.y,sz+d.z);
        if (nattr.is_empty) goto mesh;
      }
    }
    delete(terms);
    seg.compact;
  }
  delete(segs);
}



#:sdf

//hrz plane defines the horizontal section
//vrt defines how the horizontal section gets scaled along the y-axis
//it allow creating shapes like pyramid or a solid of revolution.
void vxRadialCut(slab_t *slb, gfx_t *hrz, gfx_t *vrt, gfx_t *corona) {
  int i, j;
  U32 *phrz = hrz->data;
  U32 *pvrt = vrt->data;
  U32 *pcor = corona->data;
  U32 hw = hrz->w;
  U32 hh = hrz->h;

  float *af = malloc(hw*hh*sizeof(float)); //angle field
  float *rf = gfx_radial_field(hrz, af);

  int *cem = malloc(corona->h*sizeof(int));
  for (i = 0; i < corona->h; i++) {
    cem[i] = 0;
    for (j = 0; j < corona->w; j++) {
      if (pcor[i*corona->w+j]&0xFF000000) {
        cem[i] = 1;
        break;
      }
    }
  }

  segs_t *segs = slb.svo.list_segs;
  float w05 = (float)slb->w*0.5f;
  float h05 = (float)slb->h*0.5f;
  float d05 = (float)slb->d*0.5f;
  float mwh = 1.0f/fmax(w05,h05);

#if 0
#XY(x,y) phrz[(y)*hw + (x)]
  int x,y;
  for (y = 0; y < hh; y++) {
    for (x = 0; x < hw; x++) {
      if (XY(x,y)&0xFF000000) continue;
      float d = rf[(y)*hw + (x)];
      U32 ud = d*255;
      XY(x,y) = RGB(ud,ud,ud);
    }
  }
#undef XY
  return;
#endif

  for (j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    if (!slb.vcontains(seg.xyz)) continue;
    SVOTerms *terms = seg.filled_terms;
    for (i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      if (t->c > 1) {
        seg.term_split(t,terms);
        continue;
      }
      U32 x = t.x;
      U32 y = slb->h - t.y - 1;
      U32 z = t.z;

      U32 hx = x*hrz->w/slb->w;
      U32 hy = z*hrz->h/slb->d;
      uint32_t pixel = phrz[hy*hrz->w+hx];
      uint32_t transparent = pixel&0xFF000000;
      if (transparent) {
        seg.term_erase(t);
        continue;
      }

      float radius = rf[hy*hw + hx];
      U32 vx = roundf(radius*(vrt->w-1));
      U32 vy = y*vrt->h/slb->h;

      uint32_t pixel2 = pvrt[vy*vrt->w+vx];
      uint32_t transparent2 = pixel2&0xFF000000;
      if (transparent2) {
        seg.term_erase(t);
        continue;
      }


      float angle = af[hy*hw + hx];
      if (angle >= 0.0f) {
        U32 corx = roundf(angle*(corona->w-1));
        U32 cory = y*corona->h/slb->h;
        uint32_t pixel3 = pcor[cory*corona->w+corx];
        uint32_t transparent3 = pixel3&0xFF000000;
        if (transparent3) {
          seg.term_erase(t);
          continue;
        }
      } else {
        if (cem[y*corona->h/slb->h]) {
          seg.term_erase(t);
          continue;
        }
      }
    }
    delete(terms);
    seg.compact;
  }
  delete(segs);
  free(rf);
  free(af);
  free(cem);
}

char *vxSamplePastedAABB(slab_t *slb, slab_t *brush) {
  vec3 bmin = (vec3){0.0f,0.0f,0.0f};
  vec3 bmax = (vec3){slb->w-1,slb->h-1,slb->d-1};

  //we will be transfering from brush into slab coords
  slab_t *t = brush;
  brush = slb;
  slb = t;

  vec3 mrot = vdeg2rad(slb->angle);
  vec3 brot = vdeg2rad(brush->angle);

  mat3 idm = midentity();
  mat3 mm = mscaled(mrotated_r(idm,mrot),slb->scale);
  mat3 mb = mscaled(mrotated(midentity(),brot),1.0f/brush->scale);

  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    if (!slb.vcontains(seg.xyz)) continue;
    SVOTerms *terms = seg.filled_terms;
    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      U32 color = seg.term_color(t);
      U32 sx = t.x;
      U32 sy = t.y;
      U32 sz = t.z;
      U32 c = t->c;
      U32 x,y,z;
      for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
        for (x = sx; x < sx+c; x++) {
          vec3 po, pb;
          vec3 pm = (vec3){(float)x,(float)y,(float)z};
          po = vmm(pm-slb->center,mm) + slb->xyz; //model coords reverse
          pb = vmm(po-brush->xyz,mb) + brush->center; //move to brush coords
          int xx=roundf(pb.x),yy=roundf(pb.y),zz=roundf(pb.z);
          vec3 pb2 = (vec3){xx,yy,zz};
          bmin = vmin(bmin, pb2);
          bmax = vmax(bmax, pb2+(vec3){1.0f,1.0f,1.0f});
        }
      }
    }
    delete(terms);
  }
  delete(segs);
  sprintf(slb_sample_txt, "%f %f %f %f %f %f"
         ,bmin.x, bmin.y, bmin.z
         ,bmax.x, bmax.y, bmax.z);
  return slb_sample_txt;
}

#PASTE_COLOR 1
void vxPaste(slab_t *slb, slab_t *brush, U32 flags) {
  U32 invert = flags&0x01;
  U32 paste_color;

  vec3 mrot = vdeg2rad(slb->angle);
  vec3 brot = vdeg2rad(brush->angle);

  mat3 idm = midentity();
  mat3 mm = mscaled(mrotated_r(idm,mrot),slb->scale);
  mat3 mb = mscaled(mrotated(midentity(),brot),1.0f/brush->scale);

  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    if (!slb.vcontains(seg.xyz)) continue;
    SVOTerms *terms = seg.terms;
    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      U32 color = seg.term_color(t);
      U32 sx = t.x;
      U32 sy = t.y;
      U32 sz = t.z;
      U32 c = t->c;
      U32 cuts_count = 0;
      U32 voxel_count = 0;
      U32 x,y,z;
      for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
        for (x = sx; x < sx+c; x++) {
          vec3 po, pb;
          vec3 pm = (vec3){(float)x,(float)y,(float)z};
          po = vmm(pm-slb->center,mm) + slb->xyz; //model coords reverse
          pb = vmm(po-brush->xyz,mb) + brush->center; //move to brush coords
          int xx=roundf(pb.x),yy=roundf(pb.y),zz=roundf(pb.z);
          paste_color = brush.get_color(xx,yy,zz);
          U32 cut = paste_color != NIL_COLOR;
          if (invert) cut = !cut;
          cut_body();
        }
      }
      cut_tail();
    }
    delete(terms);
    seg.compact;
  }
  delete(segs);
  vxCutOutlierTerms(slb);
}
#undef PASTE_COLOR

void vxCut(slab_t *slb, slab_t *brush, U32 flags) {
  U32 invert = flags&0x01;

  vec3 mrot = vdeg2rad(slb->angle);
  vec3 brot = vdeg2rad(brush->angle);

  mat3 idm = midentity();
  mat3 mm = mscaled(mrotated_r(idm,mrot),slb->scale);
  mat3 mb = mscaled(mrotated(midentity(),brot),1.0f/brush->scale);

  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    if (!slb.vcontains(seg.xyz)) continue;
    SVOTerms *terms = seg.filled_terms;
    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      U32 color = seg.term_color(t);
      U32 sx = t.x;
      U32 sy = t.y;
      U32 sz = t.z;
      U32 c = t->c;
      U32 cuts_count = 0;
      U32 voxel_count = 0;
      U32 x,y,z;
      for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
        for (x = sx; x < sx+c; x++) {
          vec3 po, pb;
          vec3 pm = (vec3){(float)x,(float)y,(float)z};
          po = vmm(pm-slb->center,mm) + slb->xyz; //model coords reverse
          pb = vmm(po-brush->xyz,mb) + brush->center; //move to brush coords
          int xx=roundf(pb.x),yy=roundf(pb.y),zz=roundf(pb.z);
          U32 cut = brush.get_color(xx, yy, zz) != NIL_COLOR;
          if (invert) cut = !cut;
          cut_body();
        }
      }
      cut_tail();
    }
    delete(terms);
    seg.compact;
  }
  delete(segs);
}

void vxRecolor(slab_t *slb, U32 src, U32 dst) {
  if (vxGetClayColor(slb) == src) vxSetClayColor(slb, dst);
  slb->svo.recolor(src,dst);
}

static gfx_t *dbg_gfx;
void vxDbgGfx(gfx_t *gfx) {
  dbg_gfx = gfx;
}

#VX_TX_CENTER  0x00
#VX_TX_NORMAL  0x01

#VX_TX_SPHERE   0x000
#VX_TX_CYLINDER 0x100

//modify this to to use heightmap mask, so grow and bevel will be possible
void vxEnvTexture(slab_t *slb, gfx_t *gfx, int type) {
  U32 gw = gfx->w;
  U32 gh = gfx->h;
  U32 *pixels = gfx->data;
  vec3 o = (vec3){slb->w/2,slb->h/2,slb->d/2};
  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);

    SVOTerms *terms = seg.filled_terms;

    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      U32 x = t.x;
      U32 y = t.y;
      U32 z = t.z;

      if (t->c != 1) {
        seg.term_split(t,terms);
        continue;
      }

      attr_t *attr = seg.term_attr(t);
      if (attr.is_mesh) goto mesh_term;

      if (XYZ_BORDER_OR_OUTSIDE(x,y,z,slb->w,slb->h,slb->d)) {
         goto mesh_term;
      }

      for (int k = 0; k < 6; k++) {
        ivec3 d = idirs3d6[k];
        attr_t *nattr = slb.svo.get(x+d.x,y+d.y,z+d.z);
        if (nattr.is_empty) goto mesh_term;
      }

      continue;
mesh_term:;
      vec3 p = {x,y,z};
      float dist = distance(o,p);
      vec2 polar;
      if (type&VX_TX_NORMAL) {
        vec3 n;
        U32 n32 = attr->normal;
        if (n32 >= MESH_NORMAL && !attr.is_empty) {
          U32 soft = (attr->color&SOFTNESS_MASK)>>24;
          attr_t *grain = vxRecalcNormal(slb,p,&n,soft);
        } else {
          n = vrgb2normal(n32);
        }
        polar = unit_to_polar(n);
      } else {
        polar = dist > EPS ? unit_to_polar((p-o)/dist) : (vec2){0,0};
      }
      if (type&VX_TX_CYLINDER) polar.y = (float)y/(slb->h-1);
      polar.x = 1.0f - polar.x;
      polar.y = 1.0f - polar.y;
      U32 gx = polar.x*(gw-1);
      U32 gy = polar.y*(gh-1);
      U32 color = pixels[gy*gw + gx];
      attr->color = color|(attr->color&0xFF000000);
    }
    delete(terms);
    seg.compact;
  }
  delete(segs);
}

void vxCarveHeightmap(slab_t *slb, gfx_t *gfx) {
  U32 gw = gfx->w;
  U32 gh = gfx->h;
  U32 *pixels = gfx->data;
  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);

    SVOTerms *terms = seg.filled_terms;

    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      U32 x = t.x;
      U32 y = t.y;
      U32 z = t.z;

      if (t->c != 1) {
        seg.term_split(t,terms);
        continue;
      }
      U32 gx = x*gw/slb->w;
      U32 gy = z*gh/slb->d;
      U32 color = pixels[gy*gw + gx];
      U32 gz = color&0xFF;
      int height = gz*slb->h/0xFF + 1;
      if (y > height)  {
        seg.term_erase(t);
      }
    }
    delete(terms);
    seg.compact;
  }
  delete(segs);
}
