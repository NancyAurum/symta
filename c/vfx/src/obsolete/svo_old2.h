#ifndef SVO_H
#define SVO_H

#include "common.h"
#include "vec3.h"

/*
- c=1 octs can never be forks, while c>1 octs will need only 4096 atrs at max.
  and only if the octree allows the compression of non-clay colors.
- We can use a range, instead of the FORK bit. I.e. oct>FORKS_OFF.
  That way we can index without unmasking, if topo->elts offset pre subtracted.
- We can be optimized OT_PREDEF by having pointer subtracted by 2
  in a custom list type to avoid wasting two attrs per segment.
- High-level octree layers can hold the colors themselves,
  since we don't need normals for them, unless they are used for LOD.


*/


//first two are for the empty and the filler color.
#define OT_NPREDEFS    0x02

#define OT_PRDF_BASE    (0x10000-OT_NPREDEFS)
#define OT_TERM_BASE    0x2000
#define IS_FORK(n)      ((n)<OT_TERM_BASE)
#define IS_TERM(n)      ((n)>=OT_TERM_BASE)


//is predefined color
#define IS_PRDF(n)      ((n)>=OT_PRDF_BASE)

#define OT_PRDF(n)      ((n)-OT_PRDF_BASE)
#define OT_TERM(n)      ((n)-OT_TERM_BASE)

#define OT_EMPTY       (OT_PRDF_BASE+0x00)
#define OT_CLAY        (OT_PRDF_BASE+0x01)

struct slab_t;
typedef struct slab_t slab_t;


#define TERM_NOSEG 0xFFFF

typedef struct {
  ivec3 o;   //octant x,y,z origin
  U16 c;     //octant edge size (center of the parent voxel)
  U16 node;  //parent node offset of this leaf
  U16 oct;   //octant index of this leaf
  U16 seg;
} term_t;

CXLst(SVOTerms,term_t)

typedef struct {
  U16 octs[8]; //octants (values above OT_FORK mean branching)
} node_t;

CXLst(nodes_t,node_t)


//32x32x32 segment, inside which 16-bit pointers are enough.
typedef struct seg_t {
  nodes_t topo;  //topology: nodes and links into atrs (list of U16)
  atrs_t atrs;   //attributes
  U32 fattr;     //free attribute list
} seg_t;



INLINE void seg_t.clear(int empty) {
  nodes_t *topo = &this->topo;
  atrs_t *atrs = &this->atrs;
  topo.free;
  atrs.free;
  this->fattr = 0;
  U16 *octs = topo.add->octs; //root node
  U16 value = empty ? OT_EMPTY : OT_CLAY;
  for (int i = 0; i < 8; i++) octs[i] = value;
}

seg_t *seg_t.ctor {
  this->topo.ctor;
  this->atrs.ctor;
  return this;
}

seg_t *seg_t.dtor {
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

INLINE U32 ot_t.clay_color {
  return this->predef[1].color;
}

INLINE void ot_t."clay_color="(U32 color) {
  this->predef[1].color = color;
}

INLINE attr_t *seg_t.sattr(ot_t *parent, U16 *oct) {
  attr_t *attr;
  atrs_t *atrs = &this->atrs;
  U16 ptr = *oct;
  if (!IS_PRDF(ptr)) return atrs.ref(OT_TERM(ptr));
  attr_t stub_attr = parent->predef[OT_PRDF(ptr)];
  if (this->fattr) {
    U32 aptr = this->fattr;
    *oct = aptr+OT_TERM_BASE;
    attr = atrs.ref(aptr);
    this->fattr = attr->color;
  } else {
    U32 aptr = atrs->used;
    *oct = aptr+OT_TERM_BASE;
    attr = atrs.add;
  }
  *attr = stub_attr; //init it from the stub value.
  return attr;
}

INLINE void seg_t.free_attr(U16 ptr) {
  if (IS_PRDF(ptr)) return;
  ptr = OT_TERM(ptr);
  attr_t *attr = this->atrs.ref(ptr);
  attr->color = this->fattr;
  this->fattr = ptr;
}


INLINE U32 ot_t.xyz2seg(U32 x, U32 y, U32 z) {
#if 0
  U32 c2 = this->c << 1;
  if (x >= c2 || y >= c2 || z >= c2) {
    fprintf(stdout, "ot_t.xyz2seg(%d,%d,%d): out of range\n", x, y, z);
    exit(-1);
  }
#endif
  U32 d = this->c >> 4;
  x >>= 5; y >>= 5; z >>= 5; //div by 32
  return x + y*d + z*d*d;
}

INLINE seg_t *ot_t.seg(U32 x, U32 y, U32 z) {
  U32 segid = this.xyz2seg(x,y,z);
  return this->segs + segid;
}

INLINE int ot_t.nsegs {
  U32 d = ((this->c*2)+31)>>5;
  return d*d*d;
}

INLINE void ot_t.alloc_segs {
  int n = this.nsegs;
  seg_t *segs = malloc(n*sizeof(seg_t));
  this->segs = segs;
  for (int i = 0; i < n; i++) {
    segs[i].ctor;
  }
}

INLINE void ot_t.free_segs {
  int n = this.nsegs;
  seg_t *segs = this->segs;
  for (int i = 0; i < n; i++) {
    segs[i].dtor;
  }
  free(this->segs);
  this->segs = 0;
}


INLINE void ot_t.clear_segs(int empty) {
  int n = this.nsegs;
  seg_t *segs = this->segs;
  for (int i = 0; i < n; i++) {
    segs[i].clear(empty);
  }
}

static void ot_presplit_hierarchy(ot_t *this);

INLINE void ot_t.clear(U32 color) {
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
  U16 *octs = topo.add->octs; //root node
  if (this->c == 32) {
    //these will be segment ids
    for (int i = 0; i < 8; i++) octs[i] = OT_TERM_BASE+i;
  } else {
    U16 value = color==NIL_COLOR ? OT_EMPTY : OT_CLAY;
    for (int i = 0; i < 8; i++) octs[i] = value;
  }
  this.clear_segs(color==NIL_COLOR);
  ot_presplit_hierarchy(this);
}

INLINE void ot_t.init(void *slb, U32 scale) {
  this->topo.ctor;
  this->segs = 0;
  this->c = 1<<scale;
  this.clear(NIL_COLOR);
}

INLINE void ot_t.free {
  this->topo.free;
  this.free_segs;
}

#define GRID_MOD_XYZ(x,y,z) do {x&=0x1F; y&=0x1F; z&=0x1F;} while(0)

INLINE attr_t *seg_t.get(U32 c, ot_t *parent, U32 x, U32 y, U32 z) {
  U16 ptr = 0;
  nodes_t *topo = &this->topo;
  for(;;) {
    U16 *octs = topo.ref(ptr)->octs;
    U32 oct = 0;
    if (x >= c) {oct  = 0x1; x -= c;}
    if (y >= c) {oct |= 0x2; y -= c;}
    if (z >= c) {oct |= 0x4; z -= c;}
    ptr = octs[oct];
    if (IS_FORK(ptr)) {
      c>>=1;
      continue;
    }
    if (IS_PRDF(ptr)) return &parent->predef[OT_PRDF(ptr)];
    return this->atrs.ref(OT_TERM(ptr));
  }
}


INLINE attr_t *ot_t.get(U32 x, U32 y, U32 z) {
  seg_t *seg = this.seg(x,y,z);
  if (this->c < 32) return seg.get(this->c,this,x,y,z);
  GRID_MOD_XYZ(x,y,z);
  return seg.get(16,this,x,y,z);
}

INLINE attr_t *ot_t.getf(vec3 p) {
#if 1
  return this.get(p.x,p.y,p.z);
#else
  float c = this->c;
  vec3 vc = {c,c,c};
  static ivec3 oct_bits = {0x1,0x2,0x4}; //x,y,z
  nodes_t *topo = &this->topo;
  U32 ptr = 0;
  for(;;) {
    U16 *octs = topo.ref(ptr)->octs;
    S32 oct = isum(oct_bits * -(p >= vc));
    ptr = octs[oct];
    if (IS_TERM(ptr)) return this->attr.ref(ptr);
    ptr = OT_FPTR(ptr);
    p -= oct2gt[oct]*vc;
    vc *= 0.5;
  }
#endif
}

#define SET_NORMAL    0
#define SET_ERASE     1
#define SET_EXISTING  2

INLINE attr_t *seg_t.set(ot_t *parent, U32 c, U32 x, U32 y, U32 z, int type) {
  nodes_t *topo = &this->topo;
  U16 *octs = topo.ref(0)->octs;
  for (;;) {
    U32 oct = 0;
    if (x >= c) {oct  = 0x1; x -= c;}
    if (y >= c) {oct |= 0x2; y -= c;}
    if (z >= c) {oct |= 0x4; z -= c;}
    c >>= 1;
    if (!c) {
      if (type == SET_ERASE) {
        this.free_attr(octs[oct]);
        octs[oct] = OT_EMPTY;
        parent->nmods++;
        return 0;
      }
      if (type == SET_EXISTING && octs[oct] == OT_EMPTY) return 0;
      return this.sattr(parent, &octs[oct]);
    }
    U16 ptr = octs[oct];
    if (IS_FORK(ptr)) {
      octs = topo.ref(ptr)->octs;
      continue;
    }
    parent->nmods++;
    octs[oct] = topo.used; //after `add` the octs link will be invalid
    octs = topo.add->octs;
    for (int i = 0; i < 8; i++) octs[i] = ptr;
  }
}

INLINE attr_t *ot_t.set1(U32 x, U32 y, U32 z, int type) {
  U32 c = this->c;
  nodes_t *topo = &this->topo;
  U16 *octs = topo.ref(0)->octs;
  U32 segid = this.xyz2seg(x,y,z);
  seg_t *seg = this->segs + segid;
  if (c > 16) for (;;) {
    U32 oct = 0;
    if (x >= c) {oct  = 0x1; x -= c;}
    if (y >= c) {oct |= 0x2; y -= c;}
    if (z >= c) {oct |= 0x4; z -= c;}
    c >>= 1;
    if (c == 16) {
      octs[oct] = segid+OT_TERM_BASE;
      break;
    }
    U32 ptr = octs[oct];
    if (IS_FORK(ptr)) {
      octs = topo.ref(ptr)->octs;
      continue;
    }
    this->nmods++;
    octs[oct] = topo.used; //after `add` the octs link will be invalid
    octs = topo.add->octs;
    for (int i = 0; i < 8; i++) octs[i] = ptr;
  }
  return seg.set(this,c,x,y,z,type);
}

INLINE attr_t *ot_t.set(U32 x, U32 y, U32 z) {
  return this.set1(x,y,z,SET_NORMAL);
}

INLINE void ot_t.erase(U32 x, U32 y, U32 z) {
  this.set1(x,y,z,SET_ERASE);
}

INLINE attr_t *ot_t.set_existing(U32 x, U32 y, U32 z) {
  return this.set1(x,y,z,SET_EXISTING);
}


#define TERM_ANY 0
#define TERM_MESH 1
#define TERM_FILLED 2

INLINE void seg_terms(seg_t *this, U32 segid, SVOTerms *terms
                    ,U32 nptr, U32 c
                    ,U32 x, U32 y, U32 z
                    ,int type)
{
  U16 *octs = this->topo.ref(nptr)->octs;
  for (int i = 0; i < 8; i++) {
    U16 ptr = octs[i];
    U32 ox=x,oy=y,oz=z;
    if (i&0x1) ox += c;
    if (i&0x2) oy += c;
    if (i&0x4) oz += c;
    if (IS_FORK(ptr)) {
      seg_terms(this, segid, terms, ptr, c>>1, ox, oy, oz, type);
      continue;
    }
    if (type==TERM_MESH) {
      if (c>1 || IS_PRDF(ptr)) continue;
      attr_t *attr = this->atrs.ref(OT_TERM(ptr));
      if (!attr.is_mesh) continue;
    } else if (type==TERM_FILLED) {
      if (ptr==OT_EMPTY) continue;
    }
    term_t *t = terms.add;
    t->o.x = ox;
    t->o.y = oy;
    t->o.z = oz;
    t->c = c;
    t->node = nptr;
    t->oct = i;
    t->seg = segid;
  }
}


INLINE void ot_terms(ot_t *this, SVOTerms *terms
                    ,U16 nptr, U32 c
                    ,U32 x, U32 y, U32 z
                    ,int type)
{
  U16 *octs = this->topo.ref(nptr)->octs;
  for (int i = 0; i < 8; i++) {
    U16 ptr = octs[i];
    U32 ox=x,oy=y,oz=z;
    if (i&0x1) ox += c;
    if (i&0x2) oy += c;
    if (i&0x4) oz += c;
    if (IS_FORK(ptr)) {
      ot_terms(this, terms, ptr, c>>1, ox, oy, oz, type);
      continue;
    }
    if (!IS_PRDF(ptr)) {
      U32 segid = OT_TERM(ptr);
      seg_t *seg = this->segs + segid;
      seg_terms(seg, segid, terms, 0, c>>1, ox, oy, oz, type);
      continue;
    }
    if (type==TERM_MESH) continue;
    if (type==TERM_FILLED && ptr==OT_EMPTY) continue;
    term_t *t = terms.add;
    t->o.x = ox;
    t->o.y = oy;
    t->o.z = oz;
    t->c = c;
    t->node = nptr;
    t->oct = i;
    t->seg = TERM_NOSEG;
  }
}

INLINE SVOTerms *ot_terms1(ot_t *this, int type) {
  auto terms = new(SVOTerms);
  U32 c = this->c;
  if (c < 32) {
    U32 segid = 0;
    seg_t *seg = this->segs+0;
    seg_terms(seg, segid, terms, 0, c, 0,0,0, type);
    return terms;
  }
  ot_terms(this, terms, 0, c, 0,0,0, type);
  return terms;
}

INLINE SVOTerms *ot_t.filled_terms {
  return ot_terms1(this, TERM_FILLED);
}

INLINE SVOTerms *ot_t.mesh_terms {
  return ot_terms1(this, TERM_MESH);
}

INLINE SVOTerms *ot_t.terms {
  return ot_terms1(this, TERM_ANY);
}


INLINE U32 ot_t.exists(term_t *t) {
  if (t->seg == TERM_NOSEG)
    return this->topo.ref(t->node)->octs[t->oct]!=OT_EMPTY;
  return this->segs[t->seg].topo.ref(t->node)->octs[t->oct]!=OT_EMPTY;
}

INLINE U32 ot_t.term_color(term_t *t) {
  if (t->seg == TERM_NOSEG) {
    U32 ptr = this->topo.ref(t->node)->octs[t->oct];
    return this->predef[OT_PRDF(ptr)].color;
  }
  seg_t *seg = this->segs + t->seg;
  U16 ptr = seg->topo.ref(t->node)->octs[t->oct];
  if (IS_PRDF(ptr)) return this->predef[OT_PRDF(ptr)].color;
  return seg->atrs.ref(OT_TERM(ptr))->color;
}

INLINE void ot_t.term_set(term_t *t, U32 color) {
  seg_t *seg = this->segs + t->seg;
  U16 *octs = seg->topo.ref(t->node)->octs;
  attr_t *attr = seg.sattr(this, &octs[t->oct]);
  attr->color = color;
  //attr->normal = MESH_NORMAL; //sattr should do it
}

//FIXME: entire-subtree should be erased
INLINE void ot_t.term_erase(term_t *t) {
  this->nmods++;
  if (t->seg == TERM_NOSEG) {
    U16 *o = &this->topo.ref(t->node)->octs[t->oct];
    *o = OT_EMPTY;
    U32 c = t->c;
    //clear underlaying segments
    for (U32 z = t->o.z; z < t->o.z+c; z+=32)
      for (U32 y = t->o.y; y < t->o.y+c; y+=32)
        for (U32 x = t->o.x; x < t->o.x+c; x+=32) {
          seg_t *seg = this.seg(x,y,z);
          seg.clear(1);
        }
    return;
  }
  seg_t *seg = this->segs + t->seg;
  U16 *o = &seg->topo.ref(t->node)->octs[t->oct];
  seg.free_attr(*o);
  *o = OT_EMPTY;
}

INLINE void ot_t.term_split16(term_t *t, SVOTerms *terms) {
  U32 c2 = t->c >> 1;
  U32 sx = t->o.x;
  U32 sy = t->o.y;
  U32 sz = t->o.z;
  U32 segid;
  seg_t *seg;
  U16 node;
  U16 optr;
  nodes_t *topo;
  U16 *octs;
  if (c2 != 16) {
    segid = t->seg;
    seg = this->segs + segid;
    node = seg->topo.used;
    U16 *oct = &seg->topo.ref(t->node)->octs[t->oct];
    optr = *oct;
    *oct = node;
    topo = &seg->topo;
    octs = topo.add->octs;
  } else {
    segid = this.xyz2seg(sx,sy,sz);
    seg = this->segs + segid;
    node = 0; //root node should already exist
    U16 *oct = &this->topo.ref(t->node)->octs[t->oct];
    optr = *oct; //predef indices are expected to be 16-bit
    *oct = segid+OT_TERM_BASE;
    topo = &seg->topo;
    octs = topo.ref(0)->octs;
  }
  for (int i = 0; i < 8; i++) {
    U32 ox=sx,oy=sy,oz=sz;
    if (i&0x1) ox += c2;
    if (i&0x2) oy += c2;
    if (i&0x4) oz += c2;
    term_t *nt = terms.add;
    nt->o.x = ox;
    nt->o.y = oy;
    nt->o.z = oz;
    nt->c = c2;
    nt->node = node;
    nt->oct = i;
    nt->seg = segid;
    octs[nt->oct] = optr;
  }
}

INLINE void ot_t.term_split(term_t *t, SVOTerms *terms) {
  this->nmods++;
  if (t->c <= 32) {
    this.term_split16(t,terms);
    return;
  }
  nodes_t *topo = &this->topo;
  U16 *oct = &topo.ref(t->node)->octs[t->oct];
  U32 optr = *oct;
  U32 node = topo->used;
  *oct = node;
  U16 *octs = topo.add->octs;
  U32 c2 = t->c >> 1;
  U32 sx = t->o.x;
  U32 sy = t->o.y;
  U32 sz = t->o.z;
  for (int i = 0; i < 8; i++) {
    U32 ox=sx,oy=sy,oz=sz;
    if (i&0x1) ox += c2;
    if (i&0x2) oy += c2;
    if (i&0x4) oz += c2;
    term_t *nt = terms.add;
    nt->o.x = ox;
    nt->o.y = oy;
    nt->o.z = oz;
    nt->c = c2;
    nt->node = node;
    nt->oct = i;
    nt->seg = TERM_NOSEG;
    octs[nt->oct] = optr;
  }
}

static void ot_presplit_hierarchy(ot_t *this) {
  if (this->c > 32) {
    ot_presplit_hierarchy(this);
  }
  SVOTerms *terms = slb.svo.terms;
}

static U32 ot_optimize(U32 clay_color, atrs_t *atrs
                      ,nodes_t *RESTRICT topo, U16 nptr) {
  U16 *octs = topo.ref(nptr)->octs;
  for(int oct = 0; oct < 8; oct++) {
    U16 ptr = octs[oct];
    if (IS_PRDF(ptr)) continue;
    if (IS_TERM(ptr)) {
      attr_t *attr = atrs.ref(OT_TERM(ptr));
      if (clay_color == attr->color) {
        octs[oct] = OT_CLAY;
      }
      continue;
    }
    U32 r = ot_optimize(clay_color, atrs, topo, ptr);
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

INLINE void ot_t.optimize {
  int n = this.nsegs;
  seg_t *segs = this->segs;
  for (int i = 0; i < n; i++) {
    seg_t *seg = segs+i;
    ot_optimize(this.clay_color, &seg->atrs, &seg->topo, 0);
  }
}

static U16 ot_realloc(nodes_t *RESTRICT otopo, atrs_t *RESTRICT oatrs
                          ,nodes_t *RESTRICT itopo, atrs_t *RESTRICT iatrs
                          ,U16 nptr) {
  U16 *octs = itopo.ref(nptr)->octs;
  U16 onode = otopo.reserve;
  for(int oct = 0; oct < 8; oct++) {
    U16 ptr = octs[oct];
    if (IS_FORK(ptr)) {
      ptr = ot_realloc(otopo, oatrs, itopo, iatrs, ptr);
    } else if (IS_PRDF(ptr)) {
    } else {
      U16 aptr = oatrs->used;
      attr_t *attr = oatrs.add;
      *attr = *iatrs.ref(OT_TERM(ptr));
      ptr = aptr+OT_TERM_BASE;
    }
    otopo.ref(onode)->octs[oct] = ptr;
  }
  return onode;
}

INLINE void seg_t.realloc {
  nodes_t itopo = this->topo;
  atrs_t iatrs = this->atrs;
  this->topo.ctor;
  this->atrs.ctor;
  this.clear(1);
  this->topo.used = 0;
  ot_realloc(&this->topo, &this->atrs, &itopo, &iatrs, 0);
  itopo.free;
  iatrs.free;
}

INLINE void ot_t.realloc {
  this->nmods = 0;
  int n = this.nsegs;
  seg_t *segs = this->segs;
  for (int i = 0; i < n; i++) {
    seg_t *seg = segs+i;
    seg.realloc;
  }
}

INLINE void seg_t.clone(seg_t *src) {
  this->topo.copy(&src->topo);
  this->atrs.copy(&src->atrs);
  this->fattr = src->fattr;
}

INLINE void ot_t.clone(ot_t *src) {
  this->topo.copy(&src->topo);
  seg_t *osegs = this->segs;
  seg_t *isegs = src->segs;
  int n = this.nsegs;
  for (int i = 0; i < n; i++) {
    osegs[i].clone(&isegs[i]);
  }
  this->c = src->c;
}

INLINE void ot_t.clear_interior(U32 clear_color) {
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


INLINE void seg_t.file_save(FILE *out) {
  fwrite(&this->fattr, 1, 4, out);
  this->topo.file_save(out);
  this->atrs.file_save(out);
}

INLINE void ot_t.file_save(FILE *out, slab_t *slb) {
  fwrite(&this->c, 1, 4, out);
  fwrite(&this->predef[1], 1, sizeof(this->predef[1]), out);
  this->topo.file_save(out);
  seg_t *segs = this->segs;
  int n = this.nsegs;
  for (int i = 0; i < n; i++) {
    segs[i].file_save(out);
  }
}

INLINE void ot_t.file_load(FILE *in, slab_t *slb) {
#if 0
  fread(&this->fattr, 1, 4, in);
  this->topo.file_load(in);
  this->atrs.file_load(in);
  //fprintf(stderr, "size=%d/%d\n",tree->used,tree->size);
#endif
}

#define OT_MAX_SCALE        23

#define make_vec3(x,y,z) ((vec3){(float)(x),(float)(y),(float)(z)})

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

typedef struct {
  void* nodeStack[OT_MAX_SCALE + 1];
  F32 tmaxStack[OT_MAX_SCALE + 1];
} svo_stack_t;


INLINE void *stack_read(svo_stack_t *this, int idx, F32 *tmax) {
  *tmax = this->tmaxStack[idx];
  return this->nodeStack[idx];
}

INLINE void stack_write(svo_stack_t *this, int idx, void* node, F32 tmax)
{
  this->nodeStack[idx] = node;
  this->tmaxStack[idx] = tmax;
}

INLINE int32_t __float_as_int(float x) { return *(int32_t*)&x; }

INLINE float __int_as_float(int32_t x) { return *(float*)&x; }

#define LOG2F(x) ((__float_as_int((float)(x)) >> 23) - 127)
#define EXP2F(x) __int_as_float(((U32)(x) + 127) << 23)

#define IS16BIT_SCALE(scale) ((scale)<scale16)

//the leaf mask in the NV algorithm is actually the non-leaf mask
void ot_t.raycastS(svo_res_t *res, svo_stack_t *stack, svo_ray_t ray) {
  seg_t *seg;
  nodes_t *stopo;
  nodes_t *topo = &this->topo;

  //scale below which nodes become 16-bit
  U32 scale16 = OT_MAX_SCALE + 5 - LOG2F((this->c<<1)+1);

  //fprintf(stdout, "%d: %d <= %d\n", this->c,OT_MAX_SCALE-1, scale16);
#if 0
  for (int i = 0; i < 32; i++) {
    fprintf(stdout, "%x: %d\n", i, LOG2F(i)+1);
  }
  exit(-1);
#endif
  const float epsilon = exp2f(-OT_MAX_SCALE);

  // Get rid of small ray direction components to avoid division by zero.
  if (fabsf(ray.dir.x) < epsilon) ray.dir.x = copysignf(epsilon, ray.dir.x);
  if (fabsf(ray.dir.y) < epsilon) ray.dir.y = copysignf(epsilon, ray.dir.y);
  if (fabsf(ray.dir.z) < epsilon) ray.dir.z = copysignf(epsilon, ray.dir.z);

  // Precompute the coefficients of tx(x), ty(y), and tz(z).
  // The octree is assumed to reside at coordinates [1, 2].
  float tx_coef = 1.0f / -fabs(ray.dir.x);
  float ty_coef = 1.0f / -fabs(ray.dir.y);
  float tz_coef = 1.0f / -fabs(ray.dir.z);
  float tx_bias = tx_coef * ray.orig.x;
  float ty_bias = ty_coef * ray.orig.y;
  float tz_bias = tz_coef * ray.orig.z;

  // Select octant mask to mirror the coordinate system so
  // that ray direction is negative along each axis.
  int octant_mask = 7;
  if (ray.dir.x > 0.0f) octant_mask ^= 1, tx_bias = 3.0f * tx_coef - tx_bias;
  if (ray.dir.y > 0.0f) octant_mask ^= 2, ty_bias = 3.0f * ty_coef - ty_bias;
  if (ray.dir.z > 0.0f) octant_mask ^= 4, tz_bias = 3.0f * tz_coef - tz_bias;

  // Initialize the active span of t-values.
  float t_min = fmaxf(fmaxf(2.0f * tx_coef - tx_bias, 2.0f*ty_coef - ty_bias),
         2.0f * tz_coef - tz_bias);
  float t_max = fminf(fminf(tx_coef-tx_bias, ty_coef-ty_bias), tz_coef-tz_bias);
  float h = t_max;
  t_min = fmaxf(t_min, 0.0f);
  t_max = fminf(t_max, 1.0f);

  // Initialize the current voxel to the first child of the root.
  int    idx              = 0; //child index
  vec3   pos              = (vec3){1.0f, 1.0f, 1.0f};
  int    scale            = OT_MAX_SCALE - 1;
  float  scale_exp2       = 0.5f; // exp2f(scale - s_max)

  if (1.5f*tx_coef - tx_bias > t_min) idx ^= 1, pos.x = 1.5f;
  if (1.5f*ty_coef - ty_bias > t_min) idx ^= 2, pos.y = 1.5f;
  if (1.5f*tz_coef - tz_bias > t_min) idx ^= 4, pos.z = 1.5f;

  void* node; //root node
  if (IS16BIT_SCALE(scale)) {
    seg = this->segs;
    stopo = &seg->topo;
    node = stopo.ref(0);
  } else {
    seg = 0;
    node = topo.ref(0);
  }

  // Traverse voxels along the ray as long as the current voxel
  // stays within the octree.
  while (scale < OT_MAX_SCALE) {
    DBG("scale: %d; child: %d; pos: %f,%f,%f\n",scale,idx,pos.x,pos.y,pos.z);

    //DBG("desc=%x\n", descriptor);

    // Determine maximum t-value of the cube by evaluating
    // tx(), ty(), and tz() at its corner.
    float tx_corner = pos.x * tx_coef - tx_bias;
    float ty_corner = pos.y * ty_coef - ty_bias;
    float tz_corner = pos.z * tz_coef - tz_bias;
    float tc_max = fminf(fminf(tx_corner, ty_corner), tz_corner);

    // permute child slots based on the mirroring
    uint32_t is_filled;
    uint32_t is_term;
    int oct_index = idx ^ octant_mask ^ 7;
    if (IS16BIT_SCALE(scale)) {
      U32 oct = ((node_t*)node)->octs[oct_index];
      is_filled = oct != OT_EMPTY;
      is_term = IS_TERM(oct);
    } else {
      U32 oct = ((node_t*)node)->octs[oct_index];
      is_filled = oct != OT_EMPTY;
      is_term = IS_PRDF(oct);
    }

    // Process voxel if the corresponding bit in valid mask is set
    // and the active t-span is non-empty.
    if (is_filled && t_min <= t_max) {
      // Terminate if the voxel is small enough.
#if 1
      if (tc_max*ray.dir_sz + ray.orig_sz >= scale_exp2) {
        //DBG("small_enough_break: %f,%f\n", tc_max,scale_exp2);
        break; // at t_min
      }
#endif
      // INTERSECT
      // Intersect active t-span with the cube and evaluate
      // tx(), ty(), and tz() at the center of the voxel.
      float tv_max = fminf(t_max, tc_max);

      // Descend to the first child if the resulting t-span is non-empty.
      if (t_min <= tv_max) {
        if (is_term) {
          //DBG("LEAF_HIT\n");
          break; //at t_min (overridden with tv_min)
        }

        if (tc_max < h) {
          //DBG("push: %d\n", scale);
          stack_write(stack, scale, node, t_max);
        }
        h = tc_max;

        //move to the child
        if (IS16BIT_SCALE(scale)) {
          U32 ptr = ((node_t*)node)->octs[oct_index];
          node = stopo.ref(ptr);
        } else {
          U16 oct = ((node_t*)node)->octs[oct_index];
          if (IS16BIT_SCALE(scale-1)) {
            seg = this->segs + OT_TERM(oct);
            stopo = &seg->topo;
            node = stopo.ref(0);
          } else {
            node = topo.ref(oct);
          }
        }

        float half = scale_exp2 * 0.5f;
        float tx_center = half * tx_coef + tx_corner;
        float ty_center = half * ty_coef + ty_corner;
        float tz_center = half * tz_coef + tz_corner;

        // Select child voxel that the ray enters first.
        idx = 0;
        scale--;
        scale_exp2 = half;

        if (tx_center > t_min) idx ^= 1, pos.x += scale_exp2;
        if (ty_center > t_min) idx ^= 2, pos.y += scale_exp2;
        if (tz_center > t_min) idx ^= 4, pos.z += scale_exp2;

        // Update active t-span.
        t_max = tv_max;
        continue;
      }
    }

    // ADVANCE
    // Step along the ray.
    int step_mask = 0;
    if (tx_corner <= tc_max) step_mask ^= 1, pos.x -= scale_exp2;
    if (ty_corner <= tc_max) step_mask ^= 2, pos.y -= scale_exp2;
    if (tz_corner <= tc_max) step_mask ^= 4, pos.z -= scale_exp2;

    //DBG("pos: %f,%f,%f\n", pos.x, pos.y, pos.z);

    // Update active t-span and flip bits of the child slot index.
    t_min = tc_max;
    idx ^= step_mask;

    // Proceed with pop if the bit flips disagree with the ray direction.
    if (idx&step_mask) {
      // POP
      // Find the highest differing bit between the two positions.
      unsigned int diff_bits = 0;
      if ((step_mask & 1) != 0)
        diff_bits |= __float_as_int(pos.x) ^ __float_as_int(pos.x+scale_exp2);
      if ((step_mask & 2) != 0)
        diff_bits |= __float_as_int(pos.y) ^ __float_as_int(pos.y+scale_exp2);
      if ((step_mask & 4) != 0)
        diff_bits |= __float_as_int(pos.z) ^ __float_as_int(pos.z+scale_exp2);

      // position of the highest bit
      scale = LOG2F(diff_bits);
      
      if(!IS16BIT_SCALE(scale)) seg = 0;

      // exp2f(scale - s_max)
      scale_exp2 = EXP2F(scale - OT_MAX_SCALE); 

      // Restore node voxel from the stack.
      //DBG("pop: %d\n", scale);
      node = stack_read(stack, scale, &t_max);

      // Round cube position and extract child slot index.
      int shx = __float_as_int(pos.x) >> scale;
      int shy = __float_as_int(pos.y) >> scale;
      int shz = __float_as_int(pos.z) >> scale;
      pos.x = __int_as_float(shx << scale);
      pos.y = __int_as_float(shy << scale);
      pos.z = __int_as_float(shz << scale);
      idx  = (shx & 1) | ((shy & 1) << 1) | ((shz & 1) << 2);

      // Prevent same node from being stored again and invalidate cached
      // child descriptor.
      h = 0.0f;
    }
  }

  // Indicate miss if we are outside the octree.
  if (scale >= OT_MAX_SCALE) { //don't need pos here
    res->t = 2.0f;
    //t_min = 2.0f;
    return;
  }

  // Output results.
  res->t = t_min;

#if 0
  // Undo mirroring of the coordinate system.
  if ((octant_mask & 1) == 0) pos.x = 3.0f - scale_exp2 - pos.x;
  if ((octant_mask & 2) == 0) pos.y = 3.0f - scale_exp2 - pos.y;
  if ((octant_mask & 4) == 0) pos.z = 3.0f - scale_exp2 - pos.z;

  res->pos.x = fminf(fmaxf(ray.orig.x + t_min * ray.dir.x, pos.x + epsilon),
         pos.x + scale_exp2 - epsilon);
  res->pos.y = fminf(fmaxf(ray.orig.y + t_min * ray.dir.y, pos.y + epsilon),
         pos.y + scale_exp2 - epsilon);
  res->pos.z = fminf(fmaxf(ray.orig.z + t_min * ray.dir.z, pos.z + epsilon),
         pos.z + scale_exp2 - epsilon);
#endif

  res->node = node;
  res->oct = idx ^ octant_mask ^ 7;
  res->scale = scale;
  res->seg = seg;
}

INLINE attr_t *ot_t.oct_attr(void *node, seg_t *seg, int oct) {
  if (seg) {
    U16 ptr = ((node_t*)node)->octs[oct];
    if (IS_PRDF(ptr)) return &this->predef[OT_PRDF(ptr)];
    return seg->atrs.ref(OT_TERM(ptr));
  }
  U16 ptr = ((node_t*)node)->octs[oct];
  return &this->predef[OT_PRDF(ptr)];
}

static svo_stack_t stack;
static int ray_sx,ray_sy;

INLINE void ot_t.raycast(rt_t *RESTRICT rt) {
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
  vec3 travel = hit.x*rt->rd; //FIXME: find a better way (at least for 64+ cube)
  ray.orig = rt->ro + travel;
  ray.orig_sz = 0.0;
  ray.dir = rt->rd*size;
  ray.dir_sz = 0.01;
  ray.orig /= size;
  ray.orig += (vec3){1.0f,1.0f,1.0f};
  svo_res_t res;
  svo.raycastS(&res, &stack, ray);
  if (res.t > 1.0f) { // No hit => sky.
    DBG("miss\n");
    return;
  }
  DBG("hit\n");

  // + hit.x is not required for 64x64x64 or larger cubes.
  rt->t = res.t*size*size + hit.x;

  //rt->t = hit.x + res.t*size*size;//double size, since we have divided ray.dir
  //rt->t = hit.x + distance(res.pos,ray.orig) * size;
  rt->attr = *svo.oct_attr(res.node, res.seg, res.oct);
}



//#define SVO_PROFILE

#ifdef SVO_PROFILE
#define SVO_PRF(x) x
static int ot_rcnt, ot_vcnt; //ray count, voxel depth count
#else
#define SVO_PRF(x)
#endif




#if 0
#define CNT_C1 0
#define CNT_EMPTY 1
#define CNT_FORK 2
#define CNT_ANY 3
#define CNT_MESH 4


INLINE U32 ot_count(ot_t *this
                    ,U32 nptr, U32 c
                    ,U32 x, U32 y, U32 z
                    ,int type)
{
  U32 count = 0;
  U16 *octs = this->topo.ref(nptr)->octs;
  for (int i = 0; i < 8; i++) {
    U32 ptr = octs[i];
    U32 ox=x,oy=y,oz=z;
    if (i&0x1) ox += c;
    if (i&0x2) oy += c;
    if (i&0x4) oz += c;
    if (type == CNT_ANY) count++;
    if (IS_FORK(ptr)) {
      if (type == CNT_FORK) count++;
      count += ot_count(this, OT_FPTR(ptr), c>>1, ox, oy, oz, type);
    } else {
      if (type == CNT_C1) {
        if (c==1 && ptr != OT_EMPTY) count++;
      } else if (type == CNT_EMPTY) {
        if (ptr == OT_EMPTY) count++;
      } else if (type == CNT_MESH && c == 1 && ptr != OT_EMPTY) {
        if (x==0 || y==0 || z==0) {
          count++;
          continue;
        }
        for (int j = 0; j < 6; j++) {
          ivec3 d = idirs3d6[j];
          if (this.get(ox+d.x, oy+d.y, oz+d.z)->color != NIL_COLOR) {
            count++;
            break;
          }
        }
        //attr_t *attr = this->attr.ref(ptr);
        //if (attr.is_mesh) count++;        
      }
    }
  }
  return count;
}

INLINE U32 ot_t.count(int type) {
  return ot_count(this,0,this->c,0,0,0,type);
}
#endif


#endif
