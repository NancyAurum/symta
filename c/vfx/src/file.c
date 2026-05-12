#:slb
#:mesh

//also save orientation: scale, center, angle, xyz
static char vxz_header[] = {'v','x', 'z', 7, 11, 2, '0','1'};

void seg_t.file_save(CFile &out, U32 flags) {
  this->fattr.srlz(out);
  this->topo.srlz(out);
  if (flags&OT_SAVE_NORMALS) {
    this->atrs.srlz(out);
  } else {
    U32s colors;
    atrs_t *atrs = &this->atrs;
    for (int i = 0; i < atrs.len; i++) {
      colors.push(atrs.ref(i)->color);
    }
    colors.srlz(out);
  }
}

void ot_t.file_save(CFile &out, slab_t *slb) {
  U32 flags = 0; //OT_SAVE_NORMALS
  out.write(&flags, 4);
  out.write(&this->predef[1], sizeof(this->predef[1]));
  seg_t *segs = this->segs;
  int n = this.nsegs;
  //fprintf(stdout, "nsegs=%d\n", n);
  for (int i = 0; i < n; i++) {
    //fprintf(stdout, "saving seg%d...\n", i);
    //fprintf(stdout, " nnodes=%d, natrs=%d\n", segs[i].topo.len, segs[i].atrs.len);
    segs[i].file_save(&out, flags);
  }
}

void vxSave(char *filename, slab_t *slb) {
  CFile out;
  if (!out.copen(filename)) return;
  vxCompact(slb); //should this be turned into option?
  out.write(vxz_header, 8);
  slb->w.srlz(out);
  slb->h.srlz(out);
  slb->d.srlz(out);
  slb->flags.srlz(out);
  vxInfo(slb).srlz(out);
  slb.svo.file_save(&out, slb);
}


void seg_t.file_load(CFile &in, U32 flags) {
  this->fattr.dsrlz(in);
  this->topo.dsrlz(in);
  if (flags&OT_SAVE_NORMALS) {
    this->atrs.dsrlz(in);
  } else {
    U32s colors;
    colors.dsrlz(in);
    atrs_t *atrs = &this->atrs;
    atrs.free;
    foreach(color, colors) {
      attr_t *attr = atrs.add;
      attr->color = color & ~MESH_LOCK;
      attr->normal = CLAY_NORMAL;
    }
  }
}

void ot_t.file_load(CFile &in, slab_t *slb) {
  U32 flags;
  in.read(&flags, 4);
  in.read(&this->predef[1], sizeof(this->predef[1]));
  this->predef[0].normal = NIL_NORMAL;
  this->predef[1].normal = CLAY_NORMAL;
  seg_t *segs = this->segs;
  int n = this.nsegs;
  for (int i = 0; i < n; i++) {
    //fprintf(stdout, "loading seg%d...\n", i);
    segs[i].file_load(&in, flags);
    //fprintf(stdout, " nnodes=%d, natrs=%d\n", segs[i].topo.len, segs[i].atrs.len);
  }
}

slab_t *vxLoad(char *filename) {
  U8 buf[512];
  U32 w,h,d,flags,infolen;
  slab_t *slb = 0;
  CFile in;
  if (!in.ropen(filename)) goto fail;
  in.read(buf, 8);
  if (memcmp(buf,vxz_header,8)) goto fail; //bad header
  w.dsrlz(in);
  h.dsrlz(in);
  d.dsrlz(in);
  flags.dsrlz(in);
  char *info = in.dsrlz_cstr; 
  slb = vxNew(w,h,d);
  slb.svo.file_load(&in, slb);
  vxSetInfo(slb, info);
  //vxSave("/Users/macbook/Documents/git/spell-of-mastery/work/slb/model2.slb", slb);
  return slb;
fail:
  if (slb) free(slb);
  return 0;
}

#TERM(x,y,z) data[(z)*w*h + (y)*w + (x)]

int vxExportCSV(char *filename, slab_t *slb) {
  int i,x,y,z;
  int w = slb->w, h = slb->h, d = slb->d;

  FILE *fptr = fopen(filename, "wb");
  if (!fptr) return 0;

	attr_t *data = malloc(w*h*d*sizeof(attr_t));
  for (i = 0; i < w*h*d; i++) {
    data[i].color = NIL_COLOR;
  }

  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    SVOTerms *terms = seg.filled_terms;
    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      U32 x = t.x;
      U32 y = t.y;
      U32 z = t.z;
      if (!(x < slb->w && y < slb->h && z < slb->d)) {
        continue;
      }

      if (t->c != 1) {
        seg.term_split(t,terms);
        continue;
      }

      TERM(x,y,z) = *seg.term_attr(t);
    }
    delete(terms);
    seg.compact;
  }
  delete(segs);

  fprintf(fptr,"%d,%d,%d\n", w, h, d);
  for (z=0; z < d; z++) {
		for (y=0; y < h; y++) {
    	for (x=0; x < w; x++) {
        attr_t *attr = &TERM(x,y,z);
        if (attr.is_empty) {
          fprintf(fptr,"\n");
          continue;
        }
        U32 n32 = attr->normal;
        U32 color = attr->color;
        if (n32 >= MESH_NORMAL) {
          vec3 p = (vec3){x,y,z};
          U32 soft = (attr->color&SOFTNESS_MASK)>>24;
          n32 = vxRecalcNormal(slb,p,0,soft)->normal;
        }
        fprintf(fptr,"%x,%x\n", color&0xFFFFFF,n32);
			}
		}
	}

  fclose(fptr);
  
  free(data);

  return 1;
}


slab_t *vxImportCSV(char *filename) {
  int i, x,y,z;
  slab_t *slb = 0;
  U32 w,h,d;

  CFile in;
  if (!in.ropen(filename)) return 0;

  char *l = in.line;

  sscanf(l, "%d,%d,%d", &w,&h,&d);
  slb = vxNew(w,h,d);

  for (z=0; z < d; z++) {
		for (y=0; y < h; y++) {
    	for (x=0; x < w; x++) {
        l = in.line;
        if (!l) continue; //EOF or too long line
        if (!*l) continue;
        U32 color;
        U32 normal;
        sscanf(l, "%x,%x", &color,&normal);
        attr_t *attr = slb.svo.set(x,y,z);
        attr->color = color;
        //attr->normal = CLAY_NORMAL;
        attr->normal = normal;
			}
		}
	}

  in.close;

  vxCompact(slb);
  //vxFillInterior(slb);

  return slb;
}

slab_t *vxImport(char *filename) {
  auto url = filename.url_parts;
  slab_t *slb = 0;
  if (url->ext.eqi("kv6")) {
    slb = vxImportKV6(filename);
  } else if (url->ext.eqi("kvx")) {
    slb = vxImportKVX(filename);
  } else if (url->ext.eqi("vxl")) {
    slb = vxImportVXL(filename);
  } else if (url->ext.eqi("csv")) {
    slb = vxImportCSV(filename);
  }
  return slb;
}
