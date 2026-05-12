#ifndef SVO_H
#define SVO_H

#include "common.h"
#include "vec3.h"



/*
TODO:
- Since the mesh terms always have c=1, we can iterate on them, instead of
  iterating on empty terms, and then checking the neigbors for empty.
- If we encode node's octs in an open array, we could get rid of the term->oct,
  using direct addressing.
- having topo->elts pointer subtracted by 0xEFFF can result in faster code.
*/

/*
#define CN_FORK      0x8000
#define IS_FORK(n)   ((n)&CN_FORK)
#define IS_TERM(n)   (!IS_FORK(n))
#define CN_FPTR(n)   ((n)&0x7FFF)
*/

#define CN_FORK      0xEFFF
#define IS_FORK(n)   ((n)>=CN_FORK)
#define IS_TERM(n)   ((n)<CN_FORK)
#define CN_FPTR(n)   ((n)-CN_FORK)


//first two are for the empty and the filler color.
#define CN_PREDEF    2
#define CN_EMPTY     0x0000
#define CN_CLAY      0x0001

struct slab_t;
typedef struct slab_t slab_t;

typedef struct {
  //FIXME: W,H,D are actually 5 bit values now, c is 4 bit, while oct is 3-bit
  //       node is 12 bits, giving a total of 34 bits.
  //       30 bits should be enough for the chunk
  //       so we can pack everything into a 64 bit integer
  ivec3 o;   //octant x,y,z origin
  U32 c;     //octant edge size (center of the parent voxel)
  U32 node;  //parent node offset of this leaf
  U32 oct;   //octant index of this leaf
  U32 chunk; //chunk this term belongs to
} term_t;

CXLst(SVOTerms,term_t)

typedef struct {
  U16 octs[8];
} node_t;

CXLst(nodes_t,node_t)

typedef struct Chunk {
  nodes_t  topo;   //topology: nodes and links into attr (list of unit32_t)
  attrs_t  attr;   //attributes
  U32 c;      //cube center x,y,z
  U32 fattr;  //free attrs list
} Chunk;


INLINE void Chunk.clear(U32 color) {
  nodes_t *topo = &this->topo;
  attrs_t *attr = &this->attr;
  topo.free;
  attr.free;
  this->fattr = 0;
  attr_t *empty = this->attr.add;
  empty->color = NIL_COLOR;
  empty->normal = NIL_NORMAL;
  attr_t *filler;
  filler = this->attr.add;
  filler->color = color;
  filler->normal = CLAY_NORMAL;
  U16 *octs = topo.add->octs; //root node
  U32 value = color!=NIL_COLOR ? CN_CLAY : CN_EMPTY;
  for (int i = 0; i < 8; i++) octs[i] = value;
}

INLINE Chunk *Chunk.init(U32 scale, U32 color) {
  this->topo.ctor;
  this->attr.ctor;
  this->c = 1<<scale;
  this.clear(color);
  return this;
}

INLINE void Chunk.free {
  this->topo.free;
  this->attr.free;
}

INLINE Chunk *Chunk.ctor {
  return this;
}

INLINE Chunk *Chunk.dtor {
  this.free;
  return this;
}

INLINE attr_t *Chunk.get(U32 x, U32 y, U32 z) {
  U32 c = this->c;
  nodes_t *topo = &this->topo;
  U32 ptr = 0;
  for(;;) {
    U16 *octs = topo.ref(ptr)->octs;
    U32 oct = 0;
    if (x >= c) {oct  = 0x1; x -= c;}
    if (y >= c) {oct |= 0x2; y -= c;}
    if (z >= c) {oct |= 0x4; z -= c;}
    ptr = octs[oct];
    if (IS_TERM(ptr)) return this->attr.ref(ptr);
    c>>=1;
    ptr = CN_FPTR(ptr);
  }
}

INLINE attr_t *Chunk.getv(vec3 p) {
  return this.get(p.x,p.y,p.z);
}

INLINE attr_t *Chunk.sattr(U16 *oct) {
  attr_t *attr;
  attrs_t *attrs = &this->attr;
  U32 ptr = *oct;
  if (ptr >= CN_PREDEF) {
    attr = attrs.ref(ptr);
  } else {
    attr_t stub_attr = *attrs.ref(ptr); //copy, since attrs.add frees ptr
    if (this->fattr) {
      U32 aptr = this->fattr;
      *oct = aptr;
      attr = attrs.ref(aptr);
      this->fattr = attr->color;
    } else {
      U32 aptr = attrs->used;
      *oct = aptr;
      attr = attrs.add;
    }
    *attr = stub_attr; //init it from the stub value.
  }
  return attr;
}

INLINE void Chunk.free_attr(U32 ptr) {
  if (ptr < CN_PREDEF) return;
  attr_t *attr = this->attr.ref(ptr);
  attr->color = this->fattr;
  this->fattr = ptr;
}

INLINE attr_t *Chunk.set1(U32 x, U32 y, U32 z, int erase) {
  U32 c = this->c;
  nodes_t *topo = &this->topo;
  U32 ptr = 0;
  U16 *octs = topo.ref(ptr)->octs;
  for (;;) {
    U32 oct = 0;
    if (x >= c) {oct  = 0x1; x -= c;}
    if (y >= c) {oct |= 0x2; y -= c;}
    if (z >= c) {oct |= 0x4; z -= c;}
    c >>= 1;
    if (!c) {
      if (erase) {
        this.free_attr(octs[oct]);
        octs[oct] = CN_EMPTY;
        return 0;
      }
      return this.sattr(&octs[oct]);
    }
    ptr = octs[oct];
    if (IS_FORK(ptr)) {
      octs = topo.ref(CN_FPTR(ptr))->octs;
      continue;
    }
    octs[oct] = topo.used+CN_FORK; //after `add` the octs link will be invalid
    octs = topo.add->octs;
    for (int i = 0; i < 8; i++) octs[i] = ptr;
  }
}

INLINE attr_t *Chunk.set(U32 x, U32 y, U32 z) {
  return this.set1(x,y,z,0);
}

INLINE void Chunk.erase(U32 x, U32 y, U32 z) {
  this.set1(x,y,z,1);
}


static void ot_mesh_terms(Chunk *this, SVOTerms *terms
                      ,U32 nptr, U32 c
                      ,U32 x, U32 y, U32 z)
{
  U16 *octs = this->topo.ref(nptr)->octs;
  for (int i = 0; i < 8; i++) {
    U32 ptr = octs[i];
    U32 ox=x,oy=y,oz=z;
    if (i&0x1) ox += c;
    if (i&0x2) oy += c;
    if (i&0x4) oz += c;
    if (IS_FORK(ptr)) {
      ot_mesh_terms(this, terms, CN_FPTR(ptr), c>>1, ox, oy, oz);
    } else {
      if (c>1) continue;
      attr_t *attr = this->attr.ref(ptr);
      if (!attr.is_mesh) continue;
      term_t *t = terms.add;
      t->o.x = ox;
      t->o.y = oy;
      t->o.z = oz;
      t->c = c;
      t->node = nptr;
      t->oct = i;
    }
  }
}

INLINE SVOTerms *Chunk.mesh_terms {
  auto terms = new(SVOTerms);
  ot_mesh_terms(this,terms,0,this->c,0,0,0);
  return terms;
}


static void ot_terms(Chunk *this, SVOTerms *terms
                    ,U32 nptr, U32 c
                    ,U32 x, U32 y, U32 z)
{
  U16 *octs = this->topo.ref(nptr)->octs;
  for (int i = 0; i < 8; i++) {
    U32 ptr = octs[i];
    U32 ox=x,oy=y,oz=z;
    if (i&0x1) ox += c;
    if (i&0x2) oy += c;
    if (i&0x4) oz += c;
    if (IS_FORK(ptr)) {
      ot_terms(this, terms, CN_FPTR(ptr), c>>1, ox, oy, oz);
    } else {
      term_t *t = terms.add;
      t->o.x = ox;
      t->o.y = oy;
      t->o.z = oz;
      t->c = c;
      t->node = nptr;
      t->oct = i;
    }
  }
}

INLINE SVOTerms *Chunk.terms {
  auto terms = new(SVOTerms);
  ot_terms(this,terms,0,this->c,0,0,0);
  return terms;
}


static U32 ot_optimize(nodes_t *RESTRICT topo, U32 nptr) {
  U16 *octs = topo.ref(nptr)->octs;
  for(int oct = 0; oct < 8; oct++) {
    U32 ptr = octs[oct];
    if (IS_TERM(ptr)) continue;
    U32 r = ot_optimize(topo, CN_FPTR(ptr));
    if (r) octs[oct] = r;
  }
  U32 v = octs[0];
  if (octs[1]==v && octs[2]==v && octs[3]==v && octs[4]==v
      && octs[5]==v && octs[6]==v && octs[7]==v)
  {
    return v;
  }
  return 0;
}

INLINE void Chunk.optimize {
  ot_optimize(&this->topo, 0);
}

static U32 ot_realloc(nodes_t *RESTRICT otopo, attrs_t *RESTRICT oattr
                          ,nodes_t *RESTRICT itopo, attrs_t *RESTRICT iattr
                          ,U32 nptr) {
  U16 *octs = itopo.ref(nptr)->octs;
  U32 onode = otopo.reserve;
  for(int oct = 0; oct < 8; oct++) {
    U32 ptr = octs[oct];
    if (IS_FORK(ptr)) {
      ptr = ot_realloc(otopo, oattr, itopo, iattr, CN_FPTR(ptr))+CN_FORK;
    } else if (ptr < CN_PREDEF) {
    } else {
      U32 aptr = oattr->used;
      attr_t *attr = oattr.add;
      *attr = *iattr.ref(ptr);
      ptr = aptr;
    }
    otopo.ref(onode)->octs[oct] = ptr;
  }
  return onode;
}

INLINE void Chunk.realloc {
  nodes_t itopo = this->topo;
  attrs_t iattr = this->attr;
  this->topo.ctor;
  this->attr.ctor;
  this.clear(iattr.ref(1)->color);
  this->topo.used = 0;
  ot_realloc(&this->topo, &this->attr, &itopo, &iattr, 0);
  itopo.free;
  iattr.free;
}

INLINE U32 Chunk.exists(term_t *t) {
  return this->topo.ref(t->node)->octs[t->oct];
}

INLINE U32 Chunk.term_color(term_t *t) {
  U32 ptr = this->topo.ref(t->node)->octs[t->oct];
  return this->attr.ref(ptr)->color;
}

INLINE void Chunk.term_set(term_t *t, U32 color) {
  U16 *octs = this->topo.ref(t->node)->octs;
  this.sattr(&octs[t->oct])->color = color;
}

INLINE void Chunk.term_set_mesh(term_t *t, U32 color) {
  U16 *octs = this->topo.ref(t->node)->octs;
  attr_t *attr = this.sattr(&octs[t->oct]);
  attr->color = color;
  attr->normal = MESH_NORMAL;
}

//FIXME: put all sub nodes onto free list
INLINE void Chunk.term_erase(term_t *t) {
  U16 *o = &this->topo.ref(t->node)->octs[t->oct];
  this.free_attr(*o);
  *o = CN_EMPTY;
}

INLINE void Chunk.term_split(term_t *t, SVOTerms *terms) {
  nodes_t *topo = &this->topo;
  U32 optr = topo.ref(t->node)->octs[t->oct];
  topo.ref(t->node)->octs[t->oct] = topo->used+CN_FORK;
  U32 node = topo.reserve;
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
    topo.ref(nt->node)->octs[nt->oct] = optr;
  }
}


INLINE Chunk *Chunk.clone(Chunk *ot) {
  this->topo.copy(&ot->topo);
  this->attr.copy(&ot->attr);
  this->c = ot->c;
  this->fattr = ot->fattr;
  return this;
}

INLINE void Chunk.clear_interior {
  nodes_t *topo = &this->topo;
  attrs_t *attrs = &this->attr;
  SVOTerms *terms = this.terms;
  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    U32 aptr = topo.ref(t->node)->octs[t->oct];
    if (aptr < CN_PREDEF) continue;
    attr_t *attr = attrs.ref(aptr);
    if (attr.is_mesh) continue;
    attr->color = this->fattr;
    this->fattr = aptr;
    topo.ref(t->node)->octs[t->oct] = CN_CLAY;
  }
}

INLINE void Chunk.set_clay_color(U32 color) {
  this->attr.ref(1)->color = color;
}

INLINE void Chunk.file_save(FILE *out) {
  fwrite(&this->fattr, 1, 4, out);
  this->topo.file_save(out);
  this->attr.file_save(out);
}

INLINE void Chunk.file_load(FILE *in) {
  fread(&this->fattr, 1, 4, in);
  this->topo.file_load(in);
  this->attr.file_load(in);
}

#define CAST_STACK_DEPTH        23
#define MAX_RAYCAST_ITERATIONS  10000

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
  node_t*     node;      // Hit parent voxel (hit_parent)
  int         childIdx;  // Hit child slot index (hit_idx)
  int         stackPtr;  // Hit scale (hit_scale)
} svo_res_t;

typedef struct {
  node_t* nodeStack[CAST_STACK_DEPTH + 1];
  F32 tmaxStack[CAST_STACK_DEPTH + 1];
} svo_stack_t;


INLINE node_t *stack_read(svo_stack_t *this, int idx, F32 *tmax) {
  *tmax = this->tmaxStack[idx];
  return this->nodeStack[idx];
}

INLINE void stack_write(svo_stack_t *this, int idx, node_t* node, F32 tmax)
{
  this->nodeStack[idx] = node;
  this->tmaxStack[idx] = tmax;
}

INLINE int32_t __float_as_int(float x) { return *(int32_t*)&x; }

INLINE float __int_as_float(int32_t x) { return *(float*)&x; }


U32 node_t.descriptor {
  U16 *octs = this->octs;
  U32 valid_mask = 0;
  U32 fork_mask = 0;
  for (int i = 0; i < 8; i++) {
    U32 ptr = octs[i];
    if (!ptr) continue;
    valid_mask |= 1<<i;
    if (IS_FORK(ptr)) fork_mask |= 1<<i;
  }
  return (valid_mask<<8) | fork_mask;
}

//the leaf mask in the NV algorithm is actually the non-leaf mask
void Chunk.raycastS(svo_res_t *res, svo_stack_t *stack, svo_ray_t ray) {
  nodes_t *topo = &this->topo;

  const float epsilon = exp2f(-CAST_STACK_DEPTH);

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
  node_t*  node           = topo.ref(0); //root node
  U32 descriptor     = 0; //invalid until fetched
  int    idx              = 0; //child index
  vec3   pos              = (vec3){1.0f, 1.0f, 1.0f};
  int    scale            = CAST_STACK_DEPTH - 1;
  float  scale_exp2       = 0.5f; // exp2f(scale - s_max)

  if (1.5f*tx_coef - tx_bias > t_min) idx ^= 1, pos.x = 1.5f;
  if (1.5f*ty_coef - ty_bias > t_min) idx ^= 2, pos.y = 1.5f;
  if (1.5f*tz_coef - tz_bias > t_min) idx ^= 4, pos.z = 1.5f;

  // Traverse voxels along the ray as long as the current voxel
  // stays within the octree.
  while (scale < CAST_STACK_DEPTH) {
    DBG("scale: %d; child: %d; pos: %f,%f,%f\n",scale,idx,pos.x,pos.y,pos.z);

    if (!descriptor) {
      descriptor = node.descriptor;
    }

    //DBG("desc=%x\n", descriptor);

    // Determine maximum t-value of the cube by evaluating
    // tx(), ty(), and tz() at its corner.
    float tx_corner = pos.x * tx_coef - tx_bias;
    float ty_corner = pos.y * ty_coef - ty_bias;
    float tz_corner = pos.z * tz_coef - tz_bias;
    float tc_max = fminf(fminf(tx_corner, ty_corner), tz_corner);

    // permute child slots based on the mirroring
    int child_shift = idx ^ octant_mask;
    int child_masks = descriptor << child_shift;
    //DBG("child_shift: %d\n", child_shift);
    //DBG("valid_child: %d\n", (child_masks & 0x8000) != 0);
    //DBG("tmin=%f, tmax=%f\n", t_min, t_max);

    // Process voxel if the corresponding bit in valid mask is set
    // and the active t-span is non-empty.
    if ((child_masks&0x8000) && t_min <= t_max) {
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
        // Terminate if the corresponding bit in the non-leaf mask is not set.
        //DBG("child_masks: %08x == %08x\n", child_masks, 0xFFFF<<6);
        //DBG("leaf_hit: %08x\n",  (0xFFFF<<6)&0x0080);
        if (!(child_masks & 0x0080)) {
          //DBG("LEAF_HIT\n");
          break; //at t_min (overridden with tv_min)
        }

        if (tc_max < h) {
          //DBG("push: %d\n", scale);
          stack_write(stack, scale, node, t_max);
        }
        h = tc_max;

        //move to the child

        //shouldn't we use `idx` instead?
        U32 ptr = CN_FPTR(node->octs[idx ^ octant_mask ^ 7]);
        node = topo.ref(ptr);

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

        // Update active t-span and invalidate cached child descriptor.
        t_max = tv_max;
        descriptor = 0;
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
      scale = (__float_as_int((float)diff_bits) >> 23) - 127;

      // exp2f(scale - s_max)
      scale_exp2 = __int_as_float((scale - CAST_STACK_DEPTH + 127) << 23); 

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
      descriptor = 0;
    }
  }

  // Indicate miss if we are outside the octree.
  //if (scale >= CAST_STACK_DEPTH) t_min = 2.0f;

  if (scale >= CAST_STACK_DEPTH) { //don't need pos here
    res->t = 2.0f;
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
  res->childIdx = idx ^ octant_mask ^ 7;
  res->stackPtr = scale;
}

INLINE attr_t *Chunk.oct_attr(node_t *node, int oct) {
  U32 ptr = node->octs[oct];
  return this->attr.ref(ptr);
}

static svo_stack_t stack;
static int ray_sx,ray_sy;

INLINE void Chunk.raycast(rt_t *RESTRICT rt) {
  rt->t = INFINITY;
  rt->attr = 0;
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
  Chunk *svo = this;
  float size = svo->c*2.0f;
  svo_ray_t ray;
  vec3 travel = hit.x*rt->rd; //FIXME: find a better way (at least for 64+ cube)
  ray.orig = rt->ro + travel;
  ray.orig_sz = 0.0;
  ray.dir = rt->rd*size;
  ray.dir_sz = 0.01;
  VDBG("ro before",ray.orig);
  ray.orig /= size;
  VDBG("ro after",ray.orig);
  //DBG("size: %f\n", size);
  ray.orig += (vec3){1.0f,1.0f,1.0f};
  svo_res_t res;
  svo.raycastS(&res, &stack, ray);

  if (res.t > 1.0f) { // No hit => sky.
    DBG("miss\n");
    return;
  }
  DBG("hit\n");
  //fprintf(stderr, "hit!\n");

  // + hit.x is not required for 64x64x64 or larger cubes.
  rt->t = res.t*size*size + hit.x;

  //rt->t = hit.x + res.t*size*size;//double size, since we have divided ray.dir
  //rt->t = hit.x + distance(res.pos,ray.orig) * size;

  node_t *node = res.node;
  rt->attr = svo.oct_attr(node, res.childIdx);
  if (!rt->attr) {
    static attr_t attr;
    attr.color = 0xFFFFFF;
    attr.normal = NIL_NORMAL;
    rt->attr = &attr;
  }
}



//#define SVO_PROFILE

#ifdef SVO_PROFILE
#define SVO_PRF(x) x
static int ot_rcnt, ot_vcnt; //ray count, voxel depth count
#else
#define SVO_PRF(x)
#endif

#endif
