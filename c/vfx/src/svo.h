//first two are for the empty and the filler color.
#OT_NPREDEFS    0x02

#OT_PRDF_BASE    (0x10000-OT_NPREDEFS)
#OT_TERM_BASE    0x2000
#IS_FORK(n)      ((n)<OT_TERM_BASE)
#IS_TERM(n)      ((n)>=OT_TERM_BASE)


//is predefined color
#IS_PRDF(n)      ((n)>=OT_PRDF_BASE)

#OT_PRDF(n)      ((n)-OT_PRDF_BASE)
#OT_TERM(n)      ((n)-OT_TERM_BASE)

#OT_EMPTY       (OT_PRDF_BASE+0x00)
#OT_CLAY        (OT_PRDF_BASE+0x01)

struct slab_t;
typedef struct slab_t slab_t;


typedef struct {
  U32 xyz;   //octant origin
  U16 c;     //octant edge size (center of the parent voxel)
  U16 node;  //parent node offset of this leaf
  U16 oct;   //octant index of this leaf
} PACKED term_t;

inline void term_t.set_xyz(int x, int y, int z) {
  this->xyz = (z<<20) | (y<<10) | x;
}

inline void term_t.set_oxyz(U32 xyz) {
  this->xyz = xyz;
}

inline S32 term_t.x {return  this->xyz     &0x3FF;}
inline S32 term_t.y {return (this->xyz>>10)&0x3FF;}
inline S32 term_t.z {return (this->xyz>>20)&0x3ff;}


TLst(SVOTerms,term_t)

typedef struct {
  U16 octs[8]; //octants (values above OT_FORK mean branching)
} node_t;

TLst(nodes_t,node_t)


//32x32x32 segment, inside which 16-bit pointers are enough.
typedef struct seg_t {
  nodes_t topo;   //topology: nodes and links into atrs (list of U16)
  atrs_t atrs;    //attributes
  U32 fattr;      //free attribute list
  ot_t *parent;   //octree this segment belongs to
  U32 visible;    //last render cycle this segment was seen
} seg_t;

TLst(segs_t,seg_t*)


inline void seg_t.clear(int empty) {
  nodes_t *topo = &this->topo;
  atrs_t *atrs = &this->atrs;
  topo.free;
  atrs.free;
  this->visible = 0;
  this->fattr = 0;
  U16 *octs = topo.add->octs; //root node
  U16 value = empty ? OT_EMPTY : OT_CLAY;
  for (int i = 0; i < 8; i++) octs[i] = value;
}

inline seg_t *seg_t.ctor {
  this->topo.ctor;
  this->atrs.ctor;
  return this;
}

inline seg_t *seg_t.dtor {
  this->topo.free;
  this->atrs.free;
  return this;
}


typedef struct ot_t {
  nodes_t topo;        //topology: nodes and links into attr (list of unit32_t)
  seg_t *segs;         //32x32x32 bottom-level 16-bit segments
  U32 c;               //cube center x,y,z
  U32 nmods;           //modification counter
  attr_t predef[OT_NPREDEFS];
} ot_t;

inline U32 ot_t.clay_color {
  return this->predef[1].color;
}

inline void ot_t."clay_color="(U32 color) {
  this->predef[1].color = color;
}

inline attr_t *seg_t.sattr(U16 *oct) {
  attr_t *attr;
  atrs_t *atrs = &this->atrs;
  U16 ptr = *oct;
  if (!IS_PRDF(ptr)) return atrs.ref(OT_TERM(ptr));
  ot_t *parent = this->parent;
  attr_t stub_attr = parent->predef[OT_PRDF(ptr)];
  if (this->fattr) {
    U32 aptr = this->fattr;
    *oct = aptr+OT_TERM_BASE;
    attr = atrs.ref(aptr);
    this->fattr = attr->color;
  } else {
    U32 aptr = atrs.len;
    *oct = aptr+OT_TERM_BASE;
    attr = atrs.add;
  }
  *attr = stub_attr; //init it from the stub value.
  return attr;
}

inline void seg_t.free_attr(U16 ptr) {
  if (IS_PRDF(ptr)) return;
  ptr = OT_TERM(ptr);
  attr_t *attr = this->atrs.ref(ptr);
  attr->color = this->fattr;
  this->fattr = ptr;
}

inline U32 seg_t.id {
  return this - this->parent->segs;
}

inline U32 ot_t.xyz2seg(U32 x, U32 y, U32 z) {
  U32 d = this->c >> 4;
  x >>= 5; y >>= 5; z >>= 5; //div by 32
  return x + y*d + z*d*d;
}

inline void ot_t.seg2xyz(U32 segid, U32 *x, U32 *y, U32 *z) {
  U32 d = (this->c+15) >> 4;
  *x = (segid%d)*32;
  segid /= d;
  *y = (segid%d)*32;
  segid /= d;
  *z = (segid%d)*32;
}


inline ivec3 seg_t.xyz {
  U32 x, y, z;
  this->parent.seg2xyz(this.id, &x, &y, &z);
  return (ivec3){x,y,z};
}



inline seg_t *ot_t.seg(U32 x, U32 y, U32 z) {
  U32 segid = this.xyz2seg(x,y,z);
  return this->segs + segid;
}

inline int ot_t.nsegs {
  U32 d = ((this->c*2)+31)>>5;
  return d*d*d;
}

inline segs_t *ot_t.list_segs {
  auto ss = new(segs_t);
  int n = this.nsegs;
  seg_t *segs = this->segs;
  for (int i = 0; i < n; i++) {
    ss.push(&segs[i]);
  }
  return ss;
}

inline void ot_t.alloc_segs {
  int n = this.nsegs;
  seg_t *segs = malloc(n*sizeof(seg_t));
  this->segs = segs;
  for (int i = 0; i < n; i++) {
    segs[i].parent = this;
    segs[i].ctor;
  }
}

inline void ot_t.free_segs {
  int n = this.nsegs;
  seg_t *segs = this->segs;
  for (int i = 0; i < n; i++) {
    segs[i].dtor;
  }
  free(this->segs);
  this->segs = 0;
}


inline void ot_t.clear_segs(int empty) {
  int n = this.nsegs;
  seg_t *segs = this->segs;
  for (int i = 0; i < n; i++) {
    segs[i].clear(empty);
  }
}

static U32 ot_t.glue_segs(U32 c, U32 sx, U32 sy, U32 sz) {
  nodes_t *topo = &this->topo;
  U32 node = topo.reserve;
  for (int i = 0; i < 8; i++) {
    U32 x=sx,y=sy,z=sz;
    if (i&0x1) x += c;
    if (i&0x2) y += c;
    if (i&0x4) z += c;
    if (c == 32) {
      topo.ref(node)->octs[i] = this.xyz2seg(x,y,z)+OT_TERM_BASE;
    } else {
      U32 child = this.glue_segs(c>>1, x, y, z);
      topo.ref(node)->octs[i] = child; //have to re-ref, since topo grows
    }
  }
  return node;
}

inline void ot_t.clear(U32 color) {
  nodes_t *topo = &this->topo;
  topo.free;
  if (this->segs) this.free_segs;
  this.alloc_segs;
  this->nmods = 0;
  attr_t *empty = &this->predef[0];
  empty->color = NIL_COLOR;
  empty->normal = NIL_NORMAL;
  attr_t *filler = &this->predef[1];
  filler->color = color;
  filler->normal = CLAY_NORMAL;
  if (this->c >= 32) { //need to init the glueing octree
    this.glue_segs(this->c, 0, 0, 0);
  }
  this.clear_segs(color==NIL_COLOR);
}

inline void ot_t.init(void *slb, U32 scale) {
  this->topo.ctor;
  this->segs = 0;
  if (scale < 4) scale = 4;
  this->c = 1<<scale;
  this.clear(NIL_COLOR);
}

inline void ot_t.free {
  this->topo.free;
  this.free_segs;
}

#GRID_MOD_XYZ(x,y,z) do {x&=0x1F; y&=0x1F; z&=0x1F;} while(0)



#SEGGETC(c,b) {
  octs = topo.ref(ptr)->octs;
  ptr = octs[(x>>b)|((y>>b)<<1)|((z>>b)<<2)];
  if (IS_TERM(ptr)) {
    if (IS_PRDF(ptr))
      return &parent->predef[OT_PRDF(ptr)];
    return this->atrs.ref(OT_TERM(ptr));
  }
  x -= x&c;
  y -= y&c;
  z -= z&c;                         
}

inline attr_t *seg_t.get16(ot_t *parent, U32 x, U32 y, U32 z) {
  U16 ptr = 0;
  nodes_t *topo = &this->topo;
  U16 *octs;
  SEGGETC(16,4)
  SEGGETC( 8,3)
  SEGGETC( 4,2)
  SEGGETC( 2,1)
  octs = topo.ref(ptr)->octs;
  ptr = octs[x | (y<<1) | (z<<2)];
  if (IS_PRDF(ptr)) return &parent->predef[OT_PRDF(ptr)];
  return this->atrs.ref(OT_TERM(ptr));
}

#SEGNILC(c,b) {
  octs = topo.ref(ptr)->octs;
  ptr = octs[(x>>b)|((y>>b)<<1)|((z>>b)<<2)];
  if (IS_TERM(ptr)) return ptr == OT_EMPTY;
  x -= x&c;
  y -= y&c;
  z -= z&c;
}

inline int seg_t.nil16(U32 x, U32 y, U32 z) {
  U16 ptr = 0;
  nodes_t *topo = &this->topo;
  U16 *octs;
  SEGNILC(16,4)
  SEGNILC( 8,3)
  SEGNILC( 4,2)
  SEGNILC( 2,1)
  octs = topo.ref(ptr)->octs;
  ptr = octs[x | (y<<1) | (z<<2)];
  return ptr == OT_EMPTY;
}


#undef SEGGETC
#undef SEGNILC

inline attr_t *ot_t.get(U32 x, U32 y, U32 z) {
  seg_t *seg = this.seg(x,y,z);
  GRID_MOD_XYZ(x,y,z);
  return seg.get16(this,x,y,z);
}

//returns 1 if voxel is empty
inline int ot_t.nil(U32 x, U32 y, U32 z) {
  seg_t *seg = this.seg(x,y,z);
  GRID_MOD_XYZ(x,y,z);
  return seg.nil16(x,y,z);
}


inline attr_t *ot_t.vget(ivec3 p) {
  return this.get(p.x,p.y,p.z);
}

#SET_NORMAL    0
#SET_ERASE     1
#SET_EXISTING  2
#SET_EMPTY     3


#SEGSETC(c,b) {
  oct = (x>>b)|((y>>b)<<1)|((z>>b)<<2);
  ptr = octs[oct];
  if (IS_FORK(ptr)) octs = topo.ref(ptr)->octs;
  else {
    if (type == SET_EXISTING && ptr == OT_EMPTY) return 0;
    if (type == SET_EMPTY && ptr != OT_EMPTY) return 0;
    parent->nmods++;
    octs[oct] = topo.len;
    octs = topo.add->octs;
    for (int i = 0; i < 8; i++) octs[i] = ptr;
  }
  x -= x&c;
  y -= y&c;
  z -= z&c;
}

inline attr_t *seg_t.set16(ot_t *parent, U32 x, U32 y, U32 z, int type) {
  nodes_t *topo = &this->topo;
  U16 *octs = topo.ref(0)->octs;
  U32 oct;
  U16 ptr;
  SEGSETC(16,4)
  SEGSETC( 8,3)
  SEGSETC( 4,2)
  SEGSETC( 2,1)
  oct = x | (y<<1) | (z<<2);
  if (type == SET_ERASE) {
    if (octs[oct] == OT_EMPTY) return 0;
    this.free_attr(octs[oct]);
    octs[oct] = OT_EMPTY;
    parent->nmods++;
    return (attr_t *)1;
  }
  if (type == SET_EXISTING && octs[oct] == OT_EMPTY) return 0;
  if (type == SET_EMPTY && octs[oct] != OT_EMPTY) return 0;
  return this.sattr(&octs[oct]);
}

inline attr_t *ot_t.set1(U32 x, U32 y, U32 z, int type) {
  seg_t *seg = this.seg(x,y,z);
  GRID_MOD_XYZ(x,y,z);
  return seg.set16(this,x,y,z, type);
}

inline attr_t *ot_t.set(U32 x, U32 y, U32 z) {
  return this.set1(x,y,z,SET_NORMAL);
}

inline int ot_t.erase(U32 x, U32 y, U32 z) {
  return this.set1(x,y,z,SET_ERASE) != 0;
}

inline attr_t *ot_t.set_existing(U32 x, U32 y, U32 z) {
  return this.set1(x,y,z,SET_EXISTING);
}

inline attr_t *ot_t.set_empty(U32 x, U32 y, U32 z) {
  return this.set1(x,y,z,SET_EMPTY);
}

#TERM_ANY    0
#TERM_EMPTY  1
#TERM_FILLED 2
#TERM_MESH   3

static U32 oct_vecs[] = {
  0x00,0x10,0x4000,0x4010,0x1000000,0x1000010,0x1004000,0x1004010,
  0x00,0x08,0x2000,0x2008,0x800000,0x800008,0x802000,0x802008,
  0x00,0x04,0x1000,0x1004,0x400000,0x400004,0x401000,0x401004,
  0x00,0x02,0x800,0x802,0x200000,0x200002,0x200800,0x200802,
  0x00,0x01,0x400,0x401,0x100000,0x100001,0x100400,0x100401
};

static void seg_terms(seg_t *this, SVOTerms *terms
                    ,U32 nptr, U32 c, U32 *dxyz
                    ,U32 xyz
                    ,int type)
{
  U16 *octs = this->topo.ref(nptr)->octs;
  for (int i = 0; i < 8; i++) {
    U16 ptr = octs[i];
    U32 oxyz = xyz+dxyz[i];
    if (IS_FORK(ptr)) {
      seg_terms(this, terms, ptr, c>>1, dxyz+8, oxyz, type);
      continue;
    }
    if (type==TERM_FILLED) {
      if (ptr==OT_EMPTY) continue;
    } else if (type==TERM_EMPTY) {
      if (ptr!=OT_EMPTY) continue;
    } else if (type==TERM_MESH) {
      if (c != 1) continue;
      if (IS_PRDF(ptr)) continue;
      attr_t *attr = this->atrs.ref(OT_TERM(ptr));
      if (!attr.is_mesh) continue;
    }
    term_t *t = terms.add;
    t.set_oxyz(oxyz);
    t->c = c;
    t->node = nptr;
    t->oct = i;
  }
}

inline SVOTerms *seg_terms1(seg_t *this, int type) {
  auto terms = new(SVOTerms);
  ot_t *parent = this->parent;
  U32 segid = this.id;
  U32 x,y,z;
  parent.seg2xyz(segid,&x,&y,&z);
  seg_terms(this, terms, 0, 16, oct_vecs, x|(y<<10)|(z<<20), type);
  return terms;
}


inline SVOTerms *seg_t.terms {
  return seg_terms1(this, TERM_ANY);
}

inline SVOTerms *seg_t.empty_terms {
  return seg_terms1(this, TERM_EMPTY);
}

inline SVOTerms *seg_t.filled_terms {
  return seg_terms1(this, TERM_FILLED);
}

inline SVOTerms *seg_t.mesh_terms {
  return seg_terms1(this, TERM_MESH);
}


inline U32 seg_t.term_exists(term_t *t) {
  return this->topo.ref(t->node)->octs[t->oct]!=OT_EMPTY;
}

inline U32 seg_t.term_color(term_t *t) {
  U16 ptr = this->topo.ref(t->node)->octs[t->oct];
  if (IS_PRDF(ptr)) return this->parent->predef[OT_PRDF(ptr)].color;
  return this->atrs.ref(OT_TERM(ptr))->color;
}

inline void seg_t.term_set(term_t *t, U32 color) {
  U16 *octs = this->topo.ref(t->node)->octs;
  attr_t *attr = this.sattr(&octs[t->oct]);
  attr->color = color;
  attr->normal = CLAY_NORMAL;
}

inline void seg_t.term_erase(term_t *t) {
  this->parent->nmods++;
  U16 *o = &this->topo.ref(t->node)->octs[t->oct];
  this.free_attr(*o);
  *o = OT_EMPTY;
}

inline attr_t *seg_t.term_attr(term_t *t) {
  U16 *octs = this->topo.ref(t->node)->octs;
  return this.sattr(&octs[t->oct]);
}

inline void seg_t.term_split(term_t *t, SVOTerms *terms) {
  U32 c2 = t->c >> 1;
  U32 xyz = t->xyz;
  U32 segid = this.id;
  seg_t *seg = this;
  U16 node = seg->topo.len;
  U16 *oct = &seg->topo.ref(t->node)->octs[t->oct];
  U16 optr = *oct;
  int b = 0;
  int cc = 16;
  while (cc > c2) {
    cc >>= 1;
    b += 8;
  }
  U32 *dxyz = oct_vecs+b;
#if 0
  if (IS_FORK(optr)) {
    //FIXME: obsolete hack which made BR_GROW work
    //       normally it should never be a fork
    U16 *octs = seg->topo.ref(optr)->octs;
    for (int i = 0; i < 8; i++) {
      U32 oxyz = xyz+dxyz[i];
      term_t *nt = terms.add;
      nt.set_oxyz(oxyz);
      nt->c = c2;
      nt->node = optr;
      nt->oct = i;
    }
    return;
  }
#endif
  *oct = node;
  nodes_t *topo = &seg->topo;
  U16 *octs = topo.add->octs;
  for (int i = 0; i < 8; i++) {
    U32 oxyz = xyz+dxyz[i];
    term_t *nt = terms.add;
    nt.set_oxyz(oxyz);
    nt->c = c2;
    nt->node = node;
    nt->oct = i;
    octs[nt->oct] = optr;
  }
}


static U32 seg_t.optimize1(U32 clay_color, U16 nptr) {
  nodes_t *RESTRICT topo = &this->topo;
  atrs_t *atrs = &this->atrs;
  U16 *octs = topo.ref(nptr)->octs;
  for(int oct = 0; oct < 8; oct++) {
    U16 ptr = octs[oct];
    if (IS_PRDF(ptr)) continue;
    if (IS_TERM(ptr)) {
      attr_t *attr = atrs.ref(OT_TERM(ptr));
      if (clay_color == attr->color && !(attr->normal&0xFF)) {
        octs[oct] = OT_CLAY;
      }
      continue;
    }
    U32 r = this.optimize1(clay_color, ptr);
    if (r != 0xFFFFFFFF) octs[oct] = r;
  }
  U16 v = octs[0];
  if (octs[1]==v && octs[2]==v && octs[3]==v && octs[4]==v
      && octs[5]==v && octs[6]==v && octs[7]==v)
  {
    return v;
  }
  return 0xFFFFFFFF;
}

static void seg_t.optimize {
  this.optimize1(this->parent.clay_color, 0);
}

inline void ot_t.optimize {
  int n = this.nsegs;
  seg_t *segs = this->segs;
  for (int i = 0; i < n; i++) {
    seg_t *seg = segs+i;
    seg.optimize;
  }
}

static U16 seg_realloc(nodes_t *RESTRICT otopo, atrs_t *RESTRICT oatrs
                          ,nodes_t *RESTRICT itopo, atrs_t *RESTRICT iatrs
                          ,U16 nptr) {
  U16 *octs = itopo.ref(nptr)->octs;
  U16 onode = otopo.reserve;
  for(int oct = 0; oct < 8; oct++) {
    U16 ptr = octs[oct];
    if (IS_FORK(ptr)) {
      ptr = seg_realloc(otopo, oatrs, itopo, iatrs, ptr);
    } else if (IS_PRDF(ptr)) {
    } else {
      U16 aptr = oatrs.len;
      attr_t *attr = oatrs.add;
      *attr = *iatrs.ref(OT_TERM(ptr));
      ptr = aptr+OT_TERM_BASE;
    }
    otopo.ref(onode)->octs[oct] = ptr;
  }
  return onode;
}

inline void seg_t.realloc {
  nodes_t itopo = this->topo;
  atrs_t iatrs = this->atrs;
  this->topo.ctor;
  this->atrs.ctor;
  this.clear(1);
  this->topo.m_used = 0;
  seg_realloc(&this->topo, &this->atrs, &itopo, &iatrs, 0);
  itopo.free;
  iatrs.free;
}

inline void ot_t.realloc {
  this->nmods = 0;
  int n = this.nsegs;
  seg_t *segs = this->segs;
  for (int i = 0; i < n; i++) {
    seg_t *seg = segs+i;
    seg.realloc;
  }
}

inline void ot_t.compact {
  this.optimize;
  this.realloc;
}

static void seg_t.compact {
  this.optimize;
  this.realloc;
}

inline void seg_t.clone(seg_t *src) {
  this->topo.copy(&src->topo);
  this->atrs.copy(&src->atrs);
  this->fattr = src->fattr;
}

inline void ot_t.clone(ot_t *src) {
  this->topo.copy(&src->topo);
  seg_t *osegs = this->segs;
  seg_t *isegs = src->segs;
  int n = this.nsegs;
  for (int i = 0; i < n; i++) {
    osegs[i].clone(&isegs[i]);
  }
  this->c = src->c;
}

inline void seg_t.recolor(U32 src, U32 dst) {
  int n = this->atrs.len;
  for (int i = 0; i < n; i++) {
    attr_t *attr = this->atrs.ref(i);
    if ((attr->color&0xFFFFFF) == src) {
      attr->color = dst|(attr->color&0xFF000000);
    }
  }
}

inline void ot_t.recolor(U32 src, U32 dst) {
  seg_t *segs = this->segs;
  int n = this.nsegs;
  for (int i = 0; i < n; i++) {
    segs[i].recolor(src,dst);
  }
}

inline void ot_t.clear_interior(U32 clear_color) {
  fprintf(stdout, "FIXME: ot_t.clear_interior\n");
  exit(1);
#if 0
  nodes_t *topo = &this->topo;
  atrs_t *atrs = &this->attr;
  SVOTerms *terms = this.terms;
  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    U32 aptr = topo.ref(t->node)->octs[t->oct];
    if (aptr < OT_PREDEF) continue;
    attr_t *attr = atrs.ref(aptr);
    if (attr.is_mesh) continue;
    attr->color = this->fattr;
    this->fattr = aptr;
    topo.ref(t->node)->octs[t->oct] = OT_CLAY;
  }
#endif
}


class octant_t {
  U32 segid;
  U32 octid;
  U32 x,y,z,c;
}

inline attr_t *seg_t.get_oct(U32 c, U32 x, U32 y, U32 z, octant_t *o) {
  U16 ptr = 0;
  nodes_t *topo = &this->topo;
  for(;;) {
    U16 pptr = ptr;
    U16 *octs = topo.ref(ptr)->octs;
    U32 oct = 0;
    U32 sx=x,sy=y,sz=z;
    if (x >= c) {oct  = 0x1; x -= c;}
    if (y >= c) {oct |= 0x2; y -= c;}
    if (z >= c) {oct |= 0x4; z -= c;}
    ptr = octs[oct];
    if (IS_FORK(ptr)) {
      c>>=1;
      continue;
    }
    o->c = c;
    U32 cm = ~(U32)(c-1);
    o->x &= cm;
    o->y &= cm;
    o->z &= cm;
    o->octid = ((U32)pptr<<3) | oct;
    if (IS_PRDF(ptr)) return &this->parent->predef[OT_PRDF(ptr)];
    return this->atrs.ref(OT_TERM(ptr));
  }
}

inline attr_t *ot_t.get_oct(U32 x, U32 y, U32 z, octant_t *o) {
  o->x = x;
  o->y = y;
  o->z = z;
  seg_t *seg = this.seg(x,y,z);
  o->segid = seg - this->segs;
  if (this->c < 32) return seg.get_oct(this->c,x,y,z,o);
  GRID_MOD_XYZ(x,y,z);
  return seg.get_oct(16,x,y,z,o);
}

inline U32 seg_t.maxid { return this->topo.len<<3; }

inline U32 seg_t.term_octid(term_t *t) {
  return ((U32)t->node<<3) | t->oct;
}


#OT_SAVE_NORMALS 0x0001

void seg_t.file_save(CFile &out, U32 flags);
void ot_t.file_save(CFile &out, slab_t *slb);

void seg_t.file_load(CFile &in, U32 flags);
void ot_t.file_load(CFile &in, slab_t *slb);

#OT_MAX_SCALE        23

#make_vec3(x,y,z) ((vec3){(float)(x),(float)(y),(float)(z)})

typedef struct {
  vec3        orig;
  float       orig_sz; // LOD at ray origin (ray_size_bias)
  vec3        dir;
  float       dir_sz;  // LOD increase along ray (ray_size_coef)
} svo_ray_t;

typedef struct {
  float       t;         // Hit t-value (hit_t)
  vec3        pos;       // Hit position (hit_pos)
  int         iter;      // Number of iterations
  void*       node;      // Hit parent voxel (hit_parent)
  int         oct;       // Hit child slot index (hit_idx)
  int         scale;     // Hit scale (hit_scale)
  seg_t*      seg;       // Hit segment
} svo_res_t;

void svo_raycastS(ot_t *this, svo_res_t *res, svo_ray_t ray);


extern U32 vx_render_cycle;

inline int seg_t.is_visible {return this->visible == vx_render_cycle;}

inline attr_t *ot_t.oct_attr(void *node, seg_t *seg, int oct) {
#if 1
  //as of now raycaster always reaches segs
  seg->visible = vx_render_cycle;
  U16 ptr = ((node_t*)node)->octs[oct];
  if (IS_PRDF(ptr)) return &this->predef[OT_PRDF(ptr)];
  return seg->atrs.ref(OT_TERM(ptr));
#else
  if (seg) {
    seg->visible = vx_render_cycle;
    U16 ptr = ((node_t*)node)->octs[oct];
    if (IS_PRDF(ptr)) return &this->predef[OT_PRDF(ptr)];
    return seg->atrs.ref(OT_TERM(ptr));
  }
  U16 ptr = ((node_t*)node)->octs[oct];
  return &this->predef[OT_PRDF(ptr)];
#endif
}

extern int ray_sx;
extern int ray_sy;

inline void ot_t.raycast(rt_t *RESTRICT rt) {
  rt->t = INFINITY;
  rt->ird = 1.0f/(rt->rd + (vec3){FLT_MIN,FLT_MIN,FLT_MIN});

  //if (!ray_aabb_hit_test1(rt->ro, rt->ird, rt->box)) return; //miss scene box

  //vec3 box = (vec3){size,size,size}; //rt->box;
  vec2 hit = ray_box_hit_test(rt->ro, rt->rd, (vec3){0,0,0}, rt->box);
  if (hit.x == INFINITY) return;

  //if (ray_sx == 256 && ray_sy == 256) bug = 1;
  //if (ray_sx == 60 && ray_sy == 60) bug = 1;

  //NV algorithm assumes that octree resides at (1,1,1) and has scale of 1x1x1.
  //when the ray origin is out of the tree, it will underflow the stack
  DBG("----------------------------\n");
  ot_t *svo = (ot_t*)rt->svo;
  float size = svo->c*2.0f;
  svo_ray_t ray;
  ray.orig = rt->ro;
  //if (hit.x < -size) return; //object is behind camera?
  if (hit.x > 0.0f) { //rt->ro is outside of the slab cube?
    //FIXME: find a better way (at least for 64+ cube)
    ray.orig += rt->rd*hit.x;
  }
  ray.orig_sz = 0.0;
  ray.dir = rt->rd*size;
  ray.dir_sz = 0.01;
  ray.orig /= size;
  ray.orig += (vec3){1.0f,1.0f,1.0f};
  svo_res_t res;
  svo_raycastS(svo, &res, ray);

  if (res.t > 1.0f) { // No hit => sky.
    DBG("miss\n");
    return;
  }

  DBG("hit\n");

  // + hit.x is not required for 64x64x64 or larger cubes.
  rt->t = res.t*size*size;
  if (hit.x > 0.0f) {
    //account for the distance we traveled to get inside the SVO
    rt->t += hit.x;
  }

  //rt->t = hit.x + res.t*size*size;//double size, since we have divided ray.dir
  //rt->t = hit.x + distance(res.pos,ray.orig) * size;
  rt->attr = *svo.oct_attr(res.node, res.seg, res.oct);
}



//#SVO_PROFILE

#if #SVO_PROFILE
#SVO_PRF(x) x
static int ot_rcnt, ot_vcnt; //ray count, voxel depth count
#else
#SVO_PRF(x)
#endif

