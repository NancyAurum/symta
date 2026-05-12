#:slb
#:mesh


//PLY format is rather similar to the SQL database table

#PLY_ASCII   0
#PLY_BIN_LE  1
#PLY_BIN_BE  2


enum {
  PLY_INT8,
  PLY_INT16,
  PLY_INT32,
  PLY_UINT8,
  PLY_UINT16,
  PLY_UINT32,
  PLY_FLOAT32,
  PLY_FLOAT64
};

class PLYType {
  char *name;
  int id;
  int size;
}


CTOR(PLYType) {
}

static PLYType *PLYType.init(char *iname, int iid, int isize) {
  name = iname.dup;
  id = iid;
  size = isize;
  return this;
}

THsh(PLYNamedTypes,char*,PLYType*)

static PLYNamedTypes *ply_named_types;

static void ply_add_type(char *name, int id, int size) {
  ply_named_types.set(name,new(PLYType).init(name,id,size));
}


class PLYColumn { //describes a column in PLY file (property)
  int off; //offset in the property tuple
  char *name;
  PLYType *size_type; //size type for lists
  PLYType *type;
}

CTOR(PLYColumn) {
  off = 0;
  name = 0;
  size_type = 0;
  type = 0;
}

DTOR(PLYColumn) {
}

TLst(PLYColumns,PLYColumn)

class PLYTable {
  char *name;
  int nrows;
  PLYColumns cols;
  int row_size;
  F64 *rows;
  F64s list;
}

CTOR(PLYTable) {
  name = "".dup;
  nrows = 0;
  rows = 0;
}

DTOR(PLYTable) {
}

TLst(PLYTables,PLYTable*)

class PLYObject {
  char *format;
  CStrs comment;
  PLYTables *tbls;
}

CTOR(PLYObject) {
  format = "".dup;
  tbls = new(PLYTables);
}

DTOR(PLYObject) {
  for (int i = 0; i < tbls.len; i++) {
    delete(tbls.elts[i]);
  }
  delete(tbls);
}

static int ply_ready;

static void ply_init() {
  if (ply_ready) return;
  ply_ready = 1;
  ply_named_types = new(PLYNamedTypes);
  ply_add_type("int8", PLY_INT8, 1);
  ply_add_type("int16", PLY_INT16, 2);
  ply_add_type("int32", PLY_INT32, 4);
  ply_add_type("uint8", PLY_UINT8, 1);
  ply_add_type("uint16", PLY_UINT16, 2);
  ply_add_type("uint32", PLY_UINT32, 4);
  ply_add_type("float32", PLY_FLOAT32, 4);
  ply_add_type("float64", PLY_FLOAT64, 8);
  //old names for these types from the original PLY_FILES.txt
  ply_add_type("char", PLY_INT8, 1);
  ply_add_type("uchar", PLY_UINT8, 1);
  ply_add_type("short", PLY_INT16, 2);
  ply_add_type("ushort", PLY_UINT16, 2);
  ply_add_type("int", PLY_INT32, 4);
  ply_add_type("uint", PLY_UINT32, 4);
  ply_add_type("long", PLY_INT32, 4);
  ply_add_type("float", PLY_FLOAT32, 8);
  ply_add_type("double", PLY_FLOAT64, 8);
}

static PLYObject *read_header(CFile *in) {
  auto hdr = new(PLYObject);
  MGrp *cgrp = mCurGrp();
  PLYTable *table = 0;
  if (in.line.ne("ply")) return 0;
  int coloff;
  char *l;
  for (;;) {
    l = in.line;
    if (l.begins("end_header")) {
      break;
    }
    char *p = l;
    while (*p && *p != ' ') p++;
    *p = 0;
    p++;
    if (!table) { //header field?
      if (l.eq("format")) {
        hdr->format = p.dup;
      } else if (l.eq("comment")) {
        hdr->comment.push(p.dup);
      } else if (l.eq("element")) {
        goto element;
      }
      continue;
    }

    if (l.eq("element")) {
element:
      coloff = 0;
      if (table) {
        table->row_size = coloff;
      }
      table = new(PLYTable);
      hdr->tbls.push(table);
      char *q = p;
      while (*q && *q != ' ') q++;
      *q = 0;
      q++;
      table->name = p.dup;
      table->nrows = q.asS32;
    } else if (l.eq("property")) {
      PLYColumn col;
      col.ctor;
      mBegin(0);
      auto words = p.split(' ');
      mBegin(cgrp); //allocate at the level above
      if (words.elts[0].eq("list")) {
        col.name = words.elts[3].dup;
        auto t = ply_named_types.get(words.elts[1]);
        if (!t) {
          say("PLY: bad header type = ", words.elts[1]);
          return 0; //FIXME: mEnd() is not called
                    // implement freeing to the saved grp pointer
        }
        col.size_type = *t;
        t = ply_named_types.get(words.elts[2]);
        if (!t) {
          say("PLY: bad header type = ", words.elts[1]);
          return 0;
        }
        col.type = *t;
        col.off = coloff;
        coloff += sizeof(void*); //list will be a pointer
      } else {
        col.name = words.elts[1].dup;
        auto t = ply_named_types.get(words.elts[0]);
        if (!t) {
          say("PLY: bad header type = ", words.elts[0]);
          return 0;
        }
        col.type = *t;
        col.off = coloff;
        coloff += col.type->size;
      }
      table->cols.push(col);
      mEnd(); //cgrp
      mEnd(); //0
    }
  }
  return hdr;
fail:
  while (mCurGrp() != cgrp) mFreeGrp(0);
  return 0;
}

inline double string_as_number(int type_id, char *s) {
#if 0
  switch (type_id) {
  case PLY_INT8   : return s.asS8;
  case PLY_UINT8  : return s.asU8;
  case PLY_INT16  : return s.asS16;
  case PLY_UINT16 : return s.asU16;
  case PLY_INT32  : return s.asS32;
  case PLY_UINT32 : return s.asU32;
  case PLY_FLOAT32: return s.asF32;
  case PLY_FLOAT64: return s.asF64;
  }
#endif
  return s.asF64;
}

static CMesh *load_mesh(char *filename) {
  int i, j;
  char *info;

  ply_init();

  MGrp *gply = mGrp(); //group for the temporary allocated PLY memory
  mBegin(gply);

  CFile *in = new(CFile);
  if (!in.ropen(filename)) goto fail;

  PLYObject *hdr = read_header(in);
  if (!hdr) goto fail;

  //say(hdr->format);
  if (hdr->format.ne("ascii 1.0")) goto fail;

  char *l;
  foreach(t,*hdr->tbls) {
    //say(t->name,"[",t->nrows,"]");
    t->rows = mAlloc(t->nrows * t->cols.len * sizeof(F64));
    F64 *p = t->rows;
    F64s *list = &t->list;
    for (i = 0; i < t->nrows; i++) {
      l = in.line;
      mBegin(0); //FIXME: this needs a macro `with(words, l.split(' '))`
      auto words = l.split(' ');
      mBegin(gply);
      int k = 0;
      for (j = 0; j < t->cols.len; j++) {
        auto col = &t->cols.elts[j];
        if (col->size_type) {
          *p++ = list.len;
          //printf("%d\n", list.len);
          char *w = words.elts[k++];
          double nelems = string_as_number(col->size_type->id,w);
          list.push(nelems);
          int inelems = nelems;
          int nwords = words.len - k;
          if (inelems != nwords) {
            say("PLY: list is missing elems: ", inelems," != ", nwords);
          }
          for ( ; k < words.len; k++) {
            w = words.elts[k];
            double item = string_as_number(col->type->id,w);
            list.push(item);
            nelems++;
          }
        } else {
          char *w = words.elts[k++];
          // convert everything to F64
          *p++ = string_as_number(col->type->id,w);
        }
      }
      mEnd();
      mEnd();
    }
  }
  delete(in);
  in = 0;

  MGrp *result_grp = mGrp(); //group for the temporary allocated PLY memory

  mBegin(result_grp);
  
  auto mesh = new(CMesh);

  auto vertices = &mesh->vertices;
  auto faces = &mesh->faces;
  int tindex = 0;

  foreach(t,*hdr->tbls) {
    if (tindex == 2) break; // other tables are not our business.
    F64 *rows = t->rows;
    F64s *list = &t->list;
    int ncols = t->cols.len;
    for (i = 0; i < t->nrows; i++) {
      F64 *p = rows + ncols*i;
      if (tindex == 0) { //vertex table
        CVertex v;
        v.xyz = (vec3){p[0],p[1],p[2]};
        v.uv = (vec2){0,0};
        vertices.push(v);
      } else {
        CFace f;
        int idx = (U32)*p;
        f.vs[0] = list.get(idx+1);
        f.vs[1] = list.get(idx+2);
        f.vs[2] = list.get(idx+3);
        faces.push(f);
      }
    }
    tindex++;
  }

#if 0
  foreach(rt,hdr->tbls) {
    PLYTable *t = &rt;
    say(t->name,"[",t->nrows,"]");
    F64 *rows = t->rows;
    F64s *list = &t->list;
    int ncols = t->cols.len;
    for (i = 0; i < t->nrows; i++) {
      for (j = 0; j < ncols; j++) {
        auto col = &t->cols.elts[j];
        double val = rows[ncols*i + j];
        if (col->size_type) {
          int idx = (U32)val;
          printf("%g,", list.get(idx));
        } else {
          printf("%g,", val);
        }
      }
      printf("\n");
    }
  }
#endif

  mEnd(); //result_grp
  mEnd(); //gply

  delete(hdr);
  mFreeGrp(gply); // wont need it anymore

  mUnmanage(result_grp);
  mFreeGrp(result_grp);

#if 0
  for (int i = 0; i < faces.len; i++) {
    CFace *f = &faces.elts[i];
    say(f->vs[0],",",f->vs[1],",",f->vs[2]);
  }
#endif

  return mesh;

fail:
  if (in) delete(in);
  if (hdr) delete(hdr);
  mFreeGrp(gply);
  return 0;
}

int vxImportPly(slab_t *slb, char *filename) {
  auto mesh = load_mesh(filename);
  if (!mesh) return 0;
  mesh.rescale(1.0,MESH_FLIP_X);
  vxVoxelize(slb, mesh);
  vxFillInterior(slb);
  delete(mesh);
  return 1;
}
