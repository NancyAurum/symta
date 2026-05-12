#:slb
#:gfx_primitives
#:camera
#:edit

#BR_PAINT  0
#BR_NPAINT 1
#BR_CUT    2
#BR_BEVEL  3
#BR_GROW   4

U32 gsoftness;

void vxSetSoftness(slab_t *slb, int value) {
  gsoftness = value;
}

//FIXME: RB_MARGIN should probably be exposed to user
//rd*sqrt(3) + margin to get out int empty space
#RB_MARGIN (1.732051f)
//#RB_MARGIN (1.732051f+0.5f)

static float paint_margin = RB_MARGIN;
void vxPaintMargin(float margin) {
  paint_margin = margin;
}


#RB_CENTER (vec3){0.5f,0.5f,0.5f}

#RB_MIN (vec3){0.0f,0.0f,0.0f}
#RB_MAX (vec3){1.0f,1.0f,1.0f}

#RB_C0 (vec3){1.0f,1.0f,0.0f}
#RB_C1 (vec3){0.0f,1.0f,1.0f}
#RB_C2 (vec3){1.0f,0.0f,1.0f}
#RB_C3 (vec3){1.0f,0.0f,0.0f}
#RB_C4 (vec3){0.0f,1.0f,0.0f}
#RB_C5 (vec3){0.0f,0.0f,1.0f}

inline int ray_blocked(slab_t *slb, vec3 ro, vec3 rd, float dist) {
  rt_t rtt, *rt = &rtt;
  rt.init(slb);
  ro = ro + rd*paint_margin;
  rt->rd = rd;
#RB_TEST rt.shot; if (fabs(rt->t) >= dist-RB_MARGIN) return 0;
  //try for each corner
  rt->ro = ro;           RB_TEST
  rt->ro = ro+RB_MAX;    RB_TEST
  rt->ro = ro+RB_CENTER; RB_TEST
  rt->ro = ro+RB_C0;     RB_TEST
  rt->ro = ro+RB_C1;     RB_TEST
  rt->ro = ro+RB_C2;     RB_TEST
  rt->ro = ro+RB_C3;     RB_TEST
  rt->ro = ro+RB_C4;     RB_TEST
  rt->ro = ro+RB_C5;     RB_TEST
#undef RB_TEST
  return 1;
}

#:project

char *vxProjectionSample(slab_t *slb, gfx_t *gfx, int x, int y, int z) {
  project_setupG();
  fprintf(stderr, "Sampling %d,%d,%d:\n", x, y, z);
  
  U32 sx=x, sy=y, sz=z;
  vec3 p = (vec3){(float)sx,(float)sy,(float)sz};
  vec3 r;
  project_body(  );
  fprintf(stderr, "   %d,%d\n", scx,scy);

  //PROJECT_POINT(r, sx  ,sy  ,sz  );
  //fprintf(stderr, "   %f,%f,%f\n", r.x, r.y, r.z);
  return "";
}

void vxTextureCut(slab_t *slb, gfx_t *gfx, int invert) {
  project_setupG();
  uint32_t *margins = gfx_margins(gfx, !invert);
  margins[2] += margins[0];
  margins[3] += margins[1];

  segs_t *segs = slb.svo.list_segs;

  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);

    { //check if cube is inside brush margins
      ivec3 sxyz = seg.xyz;
      U32 sx = sxyz.x;
      U32 sy = sxyz.y;
      U32 sz = sxyz.z;
      U32 c = 32;
      PROJECT_CUBE(continue);
      if (ex<margins[0] || ey<margins[1] || bx>=margins[2] || by>=margins[3]) {
        continue; //miss
      }
    }

    SVOTerms *terms = seg.filled_terms;
    int nmods = 0;
    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      U32 sx = t.x;
      U32 sy = t.y;
      U32 sz = t.z;
      U32 c = t->c;

      U32 cuts_count = 0;
      U32 voxel_count = 0;

      //c=4 is the threshold where it is more efficient to project cube
      if (c > 2) {
        PROJECT_CUBE(continue);
        if (   ex <  margins[0] || ey <  margins[1]
            || bx >= margins[2] || by >= margins[3]) {
          continue; //miss
        }
        for (int scy = by; scy <= ey; scy++) {
          for (int scx = bx; scx <= ex; scx++) {
            voxel_count++;
            uint32_t pixel = pixels[scy*gw+scx];
            uint32_t transparent = pixel&0xFF000000;
            if (invert) transparent = !transparent;
            if (transparent) {
              //if (c==1) goto do_cut_term;
              cuts_count++;
              if (cuts_count != voxel_count) goto do_split_term;
            }
          }
        }
      } else {
        U32 x,y,z;
        vec3 p; //currently processed voxel x,y,z

        for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
          p.y = (float)y;
          p.z = (float)z;
          for (x = sx; x < sx+c; x++) {
            p.x = (float)x;
            voxel_count++;
            project_body(goto cut_miss);
            if (invert) transparent = !transparent;
            U32 cut = transparent;
            cut_body();
      cut_miss:
            if (cuts_count) {
              goto do_split_term;
            }
          }
        }
      }
      if (cuts_count) {
        if (cuts_count == voxel_count) {
    do_cut_term:
          nmods++;
          seg.term_erase(t);
        } else {
    do_split_term:
          nmods++;
          seg.term_split(t,terms);
        }
      }
    }
    delete(terms);
    if (nmods) seg.compact;
  }
  delete(segs);
}



void vxTextureRefineMesh(slab_t *slb, gfx_t *gfx) {
  project_setupG();
  uint32_t *margins = gfx_margins(gfx, 0);
  margins[2] += margins[0];
  margins[3] += margins[1];
  segs_t *segs = slb.svo.list_segs;
for (int j = 0; j < segs.len; j++) {
  seg_t *seg = segs.get(j);
  if (!seg.is_visible) continue;

  { //check if cube is inside brush margins
    ivec3 sxyz = seg.xyz;
    U32 sx = sxyz.x;
    U32 sy = sxyz.y;
    U32 sz = sxyz.z;
    U32 c = 32;
    PROJECT_CUBE(continue);
    if (ex<margins[0] || ey<margins[1] || bx>=margins[2] || by>=margins[3]) {
      continue; //miss
    }
  }

  SVOTerms *terms = seg.filled_terms;
  for (int i = 0; i < terms.len; i++) {
    term_t *t = terms.ref(i);
    U32 sx = t.x;
    U32 sy = t.y;
    U32 sz = t.z;
    U32 c = t->c;

    if (c == 1) {
      vec3 p = {sx,sy,sz}; //currently processed voxel x,y,z
      project_body(continue);
      if (transparent) continue;
      attr_t *attr = seg.term_attr(t);
      attr->normal = CLAY_NORMAL;

      if (XYZ_BORDER_OR_OUTSIDE(sx,sy,sz,slb->w,slb->h,slb->d)) {
        attr->normal = MESH_NORMAL;
        continue;
      }

      for (int k = 0; k < 6; k++) {
        ivec3 d = idirs3d6[k];
        attr_t *nattr = slb.svo.get(sx+d.x,sy+d.y,sz+d.z);
        if (nattr.is_empty) {
          attr->normal = MESH_NORMAL;
          break;
        }
      }
      continue;
    } //if (c == 1)

    PROJECT_CUBE(continue);
    if (ex<margins[0] || ey<margins[1] || bx>=margins[2] || by>=margins[3]) {
      continue; //miss
    }

    for (int scy = by; scy <= ey; scy++) {
      for (int scx = bx; scx <= ex; scx++) {
        uint32_t pixel = pixels[scy*gw+scx];
        uint32_t transparent = pixel&0xFF000000;

        if (transparent) continue;

        //FIXME: check if the ray from the screen hits the term.
        seg.term_split(t,terms);
        goto done_split;
      }
    }

    done_split:;
  }
  delete(terms);
}
delete(segs);
}

static int gmeshlock = 0;

static void vxTexturePaint(slab_t *slb, gfx_t *gfx, int type) {
  int meshlock = gmeshlock;
  project_setupG();
  U32 softness = gsoftness<<24;
  uint32_t *margins = gfx_margins(gfx, 0);
  margins[2] += margins[0];
  margins[3] += margins[1];
  segs_t *segs = slb.svo.list_segs;

  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    if (!seg.is_visible) continue;

    { //check if cube is inside brush margins
      ivec3 sxyz = seg.xyz;
      U32 sx = sxyz.x;
      U32 sy = sxyz.y;
      U32 sz = sxyz.z;
      U32 c = 32;
      PROJECT_CUBE(continue);
      if (ex<margins[0] || ey<margins[1] || bx>=margins[2] || by>=margins[3]) {
        continue; //miss
      }
    }

    SVOTerms *terms;
    if (type == BR_BEVEL || type == BR_GROW) terms = seg.mesh_terms;
    else terms = seg.filled_terms;
    int nsplits = 0;

    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      U32 sx = t.x;
      U32 sy = t.y;
      U32 sz = t.z;
      U32 c = t->c;

      if (c == 1) {
        vec3 p = {sx,sy,sz}; //currently processed voxel x,y,z
        project_body(continue);
        if (transparent) continue;
        //float focal_dist = distance(p,focalm);
        if (ray_blocked(slb,p,screen_dir,dist)) continue;

        attr_t *attr = seg.term_attr(t);
        if (type <= BR_NPAINT) {
          if (type == BR_PAINT) {
            attr->color = pixel | (attr->color&0xFF000000);
          } else {
            attr->color = (attr->color&~SOFTNESS_MASK)|softness;
            attr->normal = CLAY_NORMAL; //needs to be recalculated
          }
          continue;
        }
        if (type==BR_BEVEL) {
          if (meshlock && !(attr->color&MESH_LOCK)) continue;
          seg.term_erase(t);
        } else { //cut==BR_GROW
          if (meshlock && (attr->color&MESH_LOCK)) continue;
          attr->normal = CLAY_NORMAL; //ensure it stops being mesh
          attr->color &= ~SOFTNESS_MASK; //strip softness
          for (int k = 0; k < 6; k++) {
            ivec3 d = idirs3d6[k];
            int xx = sx+d.x;
            if ((U32)xx >= slb->w) continue;
            int yy = sy+d.y;
            if ((U32)yy >= slb->h) continue;
            int zz = sz+d.z;
            if ((U32)zz >= slb->d) continue;
            attr_t *nattr = slb.svo.set_empty(xx,yy,zz);
            if (!nattr) continue;
            nattr->color = pixel;
            nattr->normal = CLAY_NORMAL;
            if (meshlock) nattr->color |= MESH_LOCK;
          }
        }
        continue;
      }

      PROJECT_CUBE(continue);

      if (ex<margins[0] || ey<margins[1] || bx>=margins[2] || by>=margins[3]) {
        continue; //miss
      }

      for (int scy = by; scy <= ey; scy++) {
        for (int scx = bx; scx <= ex; scx++) {
          uint32_t pixel = pixels[scy*gw+scx];
          uint32_t transparent = pixel&0xFF000000;

          if (transparent) continue;

          //FIXME: check if the ray from the screen hits the term.
          seg.term_split(t,terms);
          nsplits++;
          goto done_split;
        }
      }
      done_split:;
    }
    delete(terms);
    if (nsplits || type == BR_BEVEL || type == BR_GROW) seg.compact;
  }
  delete(segs);
}

void vxTexture(slab_t *slb, gfx_t *gfx, int type, int invert) {
#if #PROFILE
  double TStart = nano_clock();
#endif
  if (type == BR_CUT) {
    vxTextureCut(slb, gfx, invert);
  } else {
    if (type == BR_BEVEL || type == BR_GROW) vxTextureRefineMesh(slb, gfx);
    vxTexturePaint(slb, gfx, type);
  }
#if #PROFILE
  double TEnd = nano_clock();
  fprintf(stderr, "vxProject time: %g seconds\n", TEnd-TStart);
#endif
  
  //COMPACT_CHECK(slb);
}


void vxMeshLock(slab_t *slb, int state) {
  gmeshlock = state;

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
    
      if (!gmeshlock) {
        if (c != 1) continue;
        attr_t *attr = seg.term_attr(t);
        attr->color &= ~MESH_LOCK;
        continue;
      }

      if (c != 1) {
        seg.term_split(t,terms);
        continue;
      }
      vec3 p = {sx,sy,sz}; //currently processed voxel x,y,z

      attr_t *attr = seg.term_attr(t);
      if (attr.is_mesh) {
mesh:
        attr->color |= MESH_LOCK;
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
