#ifndef GRID_H
#define GRID_H

#include "chunk.h"


CXLst(chunks_t,Chunk*)


typedef struct Grid {
  U32 c;           //center x,y,z
  U32 d;           //cube eDge size
  Chunk **m;       //map into chunks 32x32x32 chunks.
} Grid;

INLINE uint32_t Grid.nchunks {
  uint32_t d = this->d;
  return d*d*d;
}

#define forEachChunkP(chunk,grid) \
  for(Chunk **chunk=(grid)->m, **e_=(grid)->m+(grid).nchunks \
     ; chunk < e_; chunk++)

#define forEachChunk(chunk,grid) \
  for(Chunk **p_=(grid)->m, **e_=(grid)->m+(grid).nchunks \
     , *chunk; p_ < e_; p_++) if((chunk = *p_,1))

INLINE void Grid.free {
  if (this->m) {
    forEachChunk(c,this) delete(*c);
    free(this->m);
    this->m = 0;
  }
}

//FIXME: have id=0 NIL chunk, which will be replaced with real chunk on mod
INLINE void Grid.clear(U32 color) {
  this.free;
  this->m = malloc(this.nchunks*sizeof(this->m[0]));
  forEachChunkP(chunk,this) *chunk = new(Chunk).init(4,color);
}

INLINE void Grid.init(U32 scale) {
  this->c = 1<<scale;
  this->d = (this->c*2+31)/32;
  this->m = 0;
  this.clear(NIL_COLOR);
}

INLINE ivec3 Grid.chunk_xyz(U32 cnk_index) {
  U32 d = this->d;
  U32 x = (cnk_index%d) << 5;
  U32 c = cnk_index/d;
  U32 y = (c%d) << 5;
  c = c/d;
  U32 z = c << 5;
  return (ivec3){x,y,z};
}

INLINE Chunk *Grid.get_chunk(U32 x, U32 y, U32 z) {
  U32 d = this->d;
  return this->m[(z>>5)*d*d + (y>>5)*d + (x>>5)];
}

#define CNK_MXYZ(x,y,z) x&0x1f,y&0x1f,z&0x1f

#define GRD_BOUNDS_CHECK(retval) \
  {U32 d = this->c*2; \
  if (!(x < d && y < d && z < d)) { \
    static attr_t attr; \
    attr.color = NIL_COLOR; \
    attr.normal = NIL_NORMAL; \
    return retval; \
  } }


INLINE attr_t *Grid.get(U32 x, U32 y, U32 z) {
  GRD_BOUNDS_CHECK(&attr);
  Chunk *cnk = this.get_chunk(x, y, z);
  return cnk.get(CNK_MXYZ(x,y,z));
}

INLINE attr_t *Grid.getv(vec3 p) {
  return this.get(p.x,p.y,p.z);
}

INLINE attr_t *Grid.set(U32 x, U32 y, U32 z) {
  GRD_BOUNDS_CHECK(&attr);
  Chunk *cnk = this.get_chunk(x, y, z);
  return cnk.set1(CNK_MXYZ(x,y,z),0);
}

INLINE void Grid.erase(U32 x, U32 y, U32 z) {
  GRD_BOUNDS_CHECK( );
  Chunk *cnk = this.get_chunk(x, y, z);
  cnk.set1(CNK_MXYZ(x,y,z),1);
}


INLINE SVOTerms *Grid.termsS(int mesh_only) {
  auto terms = new(SVOTerms);
  forEachChunkP(pcnk,this) {
    Chunk *cnk = *pcnk;
    int start = terms->used;
    if (mesh_only) {
      ot_mesh_terms(cnk, terms, 0, cnk->c, 0, 0, 0);
    } else {
      ot_terms(cnk, terms, 0, cnk->c, 0, 0, 0);
    }
    uint32_t cnk_index = pcnk - this->m;
    ivec3 cnkxyz = this.chunk_xyz(cnk_index);
    term_t *end = terms.elts+terms->used;
    for (term_t *term = terms.elts+start; term != end; term++) {
      term->chunk = cnk_index;
      term->o += cnkxyz;
    }
  }
  return terms;
}

INLINE SVOTerms *Grid.mesh_terms {
  return this.termsS(1);
}


INLINE SVOTerms *Grid.terms {
  return this.termsS(0);
}

INLINE void Grid.optimize {
  forEachChunk(cnk,this) cnk.optimize;
}

INLINE void Grid.realloc {
  forEachChunk(cnk,this) cnk.realloc;
}

INLINE U32 Grid.exists(term_t *t) {
  return this->m[t->chunk].exists(t);
}

INLINE U32 Grid.term_color(term_t *t) {
  return this->m[t->chunk].term_color(t);
}

INLINE void Grid.term_set(term_t *t, U32 color) {
  this->m[t->chunk].term_set(t, color);
}

INLINE void Grid.term_set_mesh(term_t *t, U32 color) {
  this->m[t->chunk].term_set_mesh(t, color);
}

INLINE void Grid.term_erase(term_t *t) {
  this->m[t->chunk].term_erase(t);
}

INLINE void Grid.term_split(term_t *t, SVOTerms *terms) {
  int start = terms->used;
  U32 cnk_index = t->chunk;
  this->m[cnk_index].term_split(t,terms);
  term_t *end = terms.elts+terms->used;
  //term->o += cnkxyz; is not required, since Chunk.term_split uses relative
  for (term_t *term = terms.elts+start; term != end; term++) {
    term->chunk = cnk_index;
  }
}

INLINE void Grid.clone(Grid *ot) {
  this->d = ot->d;
  uint32_t gsz = this.nchunks*sizeof(this->m[0]);
  this->m = malloc(gsz);
  memcpy(this->m, ot->m, gsz);
  forEachChunkP(pcnk,this) *pcnk = new(Chunk).clone(*pcnk);
}

INLINE void Grid.clear_interior {
  forEachChunk(cnk,this) cnk.clear_interior;
}

INLINE void Grid.set_clay_color(U32 color) {
  forEachChunk(cnk,this) cnk.set_clay_color(color);
}

INLINE void Grid.file_save(FILE *out) {
  fwrite(&this->c, 1, 4, out);
  fwrite(&this->d, 1, 4, out);
  forEachChunk(cnk,this) cnk.file_save(out);
}

INLINE void Grid.file_load(FILE *in) {
  this.free;
  fread(&this->c, 1, 4, in);
  fread(&this->d, 1, 4, in);
  this->m = malloc(this.nchunks*sizeof(this->m[0]));
  forEachChunkP(pcnk,this) {
    Chunk *cnk = new(Chunk).init(5, NIL_COLOR);
    *pcnk = cnk;
    cnk.file_load(in);
  }
}


#endif
