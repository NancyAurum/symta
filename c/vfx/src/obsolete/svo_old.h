#ifndef SVO_H
#define SVO_H

#include "common.h"
#include "vec3.h"

/*
- We can use a range, instead of the FORK bit. I.e. oct>FORKS_OFF.
  That way we can index without unmasking, if topo->elts offset pre subtracted.

*/

#define OT_FORK      0x80000000
#define IS_FORK(n)   ((n)&OT_FORK)
#define IS_TERM(n)   (!IS_FORK(n))
#define OT_FPTR(n)   ((n)&0x7FFFFFFF)

//first two are for the empty and the filler color.
#define OT_PREDEF    2
#define OT_EMPTY     0x00
#define OT_CLAY      0x01

struct slab_t;
typedef struct slab_t slab_t;

typedef struct {
  ivec3 o;   //octant x,y,z origin
  U32 c;     //octant edge size (center of the parent voxel)
  U32 node;  //parent node offset of this leaf
  U32 oct;   //octant index of this leaf
} term_t;

CXLst(SVOTerms,term_t)

typedef struct {
  U32 octs[8]; //octants (OT_FORK bit set means branching)
} node_t;

CXLst(nodes_t,node_t)

typedef struct ot_t {
  nodes_t  topo;   //topology: nodes and links into attr (list of unit32_t)
  atrs_t  attr;   //attributes
  U32 c;      //cube center x,y,z
  U32 fattr;  //free attrs list
  U32 nmods;  //modification counter
} ot_t;


INLINE void ot_t.clear(U32 color) {
  nodes_t *topo = &this->topo;
  atrs_t *attr = &this->attr;
  topo.free;
  attr.free;
  this->fattr = 0;
  this->nmods = 0;
  attr_t *empty = this->attr.add;
  empty->color = NIL_COLOR;
  empty->normal = NIL_NORMAL;
  attr_t *filler;
  filler = this->attr.add;
  filler->color = color;
  filler->normal = CLAY_NORMAL;
  U32 *octs = topo.add->octs; //root node
  U32 value = color!=NIL_COLOR ? OT_CLAY : OT_EMPTY;
  for (int i = 0; i < 8; i++) octs[i] = value;
}

INLINE void ot_t.init(void *slb, U32 scale) {
  this->topo.ctor;
  this->attr.ctor;
  this->c = 1<<scale;
  this.clear(NIL_COLOR);
}

INLINE void ot_t.free {
  this->topo.free;
  this->attr.free;
}

INLINE attr_t *ot_t.get(U32 x, U32 y, U32 z) {
  U32 c = this->c;
  nodes_t *topo = &this->topo;
  U32 ptr = 0;
  for(;;) {
    U32 *octs = topo.ref(ptr)->octs;
    U32 oct = 0;
    if (x >= c) {oct  = 0x1; x -= c;}
    if (y >= c) {oct |= 0x2; y -= c;}
    if (z >= c) {oct |= 0x4; z -= c;}
    ptr = octs[oct];
    if (IS_TERM(ptr)) return this->attr.ref(ptr);
    c>>=1;
    ptr = OT_FPTR(ptr);
  }
}

INLINE attr_t *ot_t.getf(vec3 p) {
  float c = this->c;
  vec3 vc = {c,c,c};
  static ivec3 oct_bits = {0x1,0x2,0x4}; //x,y,z
  nodes_t *topo = &this->topo;
  U32 ptr = 0;
  for(;;) {
    U32 *octs = topo.ref(ptr)->octs;
    S32 oct = isum(oct_bits * -(p >= vc));
    ptr = octs[oct];
    if (IS_TERM(ptr)) return this->attr.ref(ptr);
    ptr = OT_FPTR(ptr);
    p -= oct2gt[oct]*vc;
    vc *= 0.5;
  }
}
INLINE attr_t *ot_t.sattr(U32 *oct) {
  attr_t *attr;
  atrs_t *attrs = &this->attr;
  U32 ptr = *oct;
  if (ptr >= OT_PREDEF) {
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

INLINE void ot_t.free_attr(U32 ptr) {
  if (ptr < OT_PREDEF) return;
  attr_t *attr = this->attr.ref(ptr);
  attr->color = this->fattr;
  this->fattr = ptr;
}

#define SET_NORMAL    0
#define SET_ERASE     1
#define SET_EXISTING  2

INLINE attr_t *ot_t.set1(U32 x, U32 y, U32 z, int type) {
  U32 c = this->c;
  nodes_t *topo = &this->topo;
  U32 *octs = topo.ref(0)->octs;
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
        this->nmods++;
        return 0;
      }
      if (type == SET_EXISTING && octs[oct] == OT_EMPTY) return 0;
      return this.sattr(&octs[oct]);
    }
    U32 ptr = ptr = octs[oct];
    if (IS_FORK(ptr)) {
      octs = topo.ref(OT_FPTR(ptr))->octs;
      continue;
    }
    this->nmods++;
    octs[oct] = topo.used|OT_FORK; //after `add` the octs link will be invalid
    octs = topo.add->octs;
    for (int i = 0; i < 8; i++) octs[i] = ptr;
  }
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


INLINE void ot_terms(ot_t *this, SVOTerms *terms
                    ,U32 nptr, U32 c
                    ,U32 x, U32 y, U32 z
                    ,int type)
{
  U32 *octs = this->topo.ref(nptr)->octs;
  for (int i = 0; i < 8; i++) {
    U32 ptr = octs[i];
    U32 ox=x,oy=y,oz=z;
    if (i&0x1) ox += c;
    if (i&0x2) oy += c;
    if (i&0x4) oz += c;
    if (IS_FORK(ptr)) {
      ot_terms(this, terms, OT_FPTR(ptr), c>>1, ox, oy, oz, type);
    } else {
      if (type==TERM_MESH) {
        if (c>1) continue;
        attr_t *attr = this->attr.ref(ptr);
        if (!attr.is_mesh) continue;
      }
      if (type==TERM_FILLED && ptr==OT_EMPTY) continue;
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

INLINE SVOTerms *ot_t.filled_terms {
  auto terms = new(SVOTerms);
  ot_terms(this,terms,0,this->c,0,0,0,TERM_FILLED);
  return terms;
}

INLINE SVOTerms *ot_t.mesh_terms {
  auto terms = new(SVOTerms);
  ot_terms(this,terms,0,this->c,0,0,0,TERM_MESH);
  return terms;
}

INLINE SVOTerms *ot_t.terms {
  auto terms = new(SVOTerms);
  ot_terms(this,terms,0,this->c,0,0,0,TERM_ANY);
  return terms;
}


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
  U32 *octs = this->topo.ref(nptr)->octs;
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


static U32 ot_optimize(atrs_t *attrs, nodes_t *RESTRICT topo, U32 nptr) {
  U32 *octs = topo.ref(nptr)->octs;
  for(int oct = 0; oct < 8; oct++) {
    U32 ptr = octs[oct];
    if (IS_TERM(ptr)) {
      attr_t *clay = attrs.ref(OT_CLAY);
      attr_t *attr = attrs.ref(ptr);
      if (clay->color == attr->color) {
        octs[oct] = OT_CLAY;
      }
      continue;
    }
    U32 r = ot_optimize(attrs, topo, OT_FPTR(ptr));
    if (r != 0xFFFFFFFF) octs[oct] = r;
  }
  U32 v = octs[0];
  if (octs[1]==v && octs[2]==v && octs[3]==v && octs[4]==v
      && octs[5]==v && octs[6]==v && octs[7]==v)
  {
    return v;
  }
  return 0xFFFFFFFF;
}

INLINE void ot_t.optimize {
  ot_optimize(&this->attr, &this->topo, 0);
}

static U32 ot_realloc(nodes_t *RESTRICT otopo, atrs_t *RESTRICT oattr
                          ,nodes_t *RESTRICT itopo, atrs_t *RESTRICT iattr
                          ,U32 nptr) {
  U32 *octs = itopo.ref(nptr)->octs;
  U32 onode = otopo.reserve;
  for(int oct = 0; oct < 8; oct++) {
    U32 ptr = octs[oct];
    if (IS_FORK(ptr)) {
      ptr = ot_realloc(otopo, oattr, itopo, iattr, OT_FPTR(ptr))|OT_FORK;
    } else if (ptr < OT_PREDEF) {
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

INLINE void ot_t.realloc {
  nodes_t itopo = this->topo;
  atrs_t iattr = this->attr;
  this->topo.ctor;
  this->attr.ctor;
  this.clear(iattr.ref(1)->color);
  this->topo.used = 0;
  ot_realloc(&this->topo, &this->attr, &itopo, &iattr, 0);
  itopo.free;
  iattr.free;
  this->nmods = 0;
}

INLINE U32 ot_t.exists(term_t *t) {
  return this->topo.ref(t->node)->octs[t->oct];
}

INLINE U32 ot_t.term_color(term_t *t) {
  U32 ptr = this->topo.ref(t->node)->octs[t->oct];
  return this->attr.ref(ptr)->color;
}

INLINE void ot_t.term_set(term_t *t, U32 color) {
  U32 *octs = this->topo.ref(t->node)->octs;
  attr_t *attr = this.sattr(&octs[t->oct]);
  attr->color = color;
  //attr->normal = MESH_NORMAL;
}

//FIXME: put all sub nodes onto free list
INLINE void ot_t.term_erase(term_t *t) {
  U32 *o = &this->topo.ref(t->node)->octs[t->oct];
  this.free_attr(*o);
  *o = OT_EMPTY;
  this->nmods++;
}

INLINE void ot_t.term_split(term_t *t, SVOTerms *terms) {
  nodes_t *topo = &this->topo;
  U32 *oct = &topo.ref(t->node)->octs[t->oct];
  U32 optr = *oct;
  U32 node = topo->used;
  *oct = node|OT_FORK;
  U32 *octs = topo.add->octs;
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
    octs[nt->oct] = optr;
  }
  this->nmods++;
}

INLINE void ot_t.clone(ot_t *ot) {
  this->topo.copy(&ot->topo);
  this->attr.copy(&ot->attr);
  this->c = ot->c;
  this->fattr = ot->fattr;
}

INLINE void ot_t.clear_interior(U32 clear_color) {
  nodes_t *topo = &this->topo;
  atrs_t *attrs = &this->attr;
  SVOTerms *terms = this.terms;
  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    U32 aptr = topo.ref(t->node)->octs[t->oct];
    if (aptr < OT_PREDEF) continue;
    attr_t *attr = attrs.ref(aptr);
    if (attr.is_mesh) continue;
    attr->color = this->fattr;
    this->fattr = aptr;
    topo.ref(t->node)->octs[t->oct] = OT_CLAY;
  }
}

INLINE U32 ot_t.clay_color {
  return this->attr.ref(1)->color;
}

INLINE void ot_t."clay_color="(U32 color) {
  this->attr.ref(1)->color = color;
}

INLINE void ot_t.file_save(FILE *out, slab_t *slb) {
  fwrite(&this->fattr, 1, 4, out);
  this->topo.file_save(out);
  this->attr.file_save(out);
}

INLINE void ot_t.file_load(FILE *in, slab_t *slb) {
  fread(&this->fattr, 1, 4, in);
  this->topo.file_load(in);
  this->attr.file_load(in);
  //fprintf(stderr, "size=%d/%d\n",tree->used,tree->size);

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


//the leaf mask in the NV algorithm is actually the non-leaf mask
void ot_t.raycastS(svo_res_t *res, svo_stack_t *stack, svo_ray_t ray) {
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

    //DBG("desc=%x\n", descriptor);

    // Determine maximum t-value of the cube by evaluating
    // tx(), ty(), and tz() at its corner.
    float tx_corner = pos.x * tx_coef - tx_bias;
    float ty_corner = pos.y * ty_coef - ty_bias;
    float tz_corner = pos.z * tz_coef - tz_bias;
    float tc_max = fminf(fminf(tx_corner, ty_corner), tz_corner);

    // permute child slots based on the mirroring
    uint32_t oct = node->octs[idx ^ octant_mask ^ 7];

    // Process voxel if the corresponding bit in valid mask is set
    // and the active t-span is non-empty.
    if (oct && t_min <= t_max) {
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
        if (IS_TERM(oct)) {
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
        U32 ptr = OT_FPTR(node->octs[idx ^ octant_mask ^ 7]);
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

INLINE attr_t *ot_t.oct_attr(node_t *node, int oct) {
  U32 ptr = node->octs[oct];
  return this->attr.ref(ptr);
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

  node_t *node = res.node;
  attr_t *pattr = svo.oct_attr(node, res.childIdx);

  rt->attr = *pattr;
}



//#define SVO_PROFILE

#ifdef SVO_PROFILE
#define SVO_PRF(x) x
static int ot_rcnt, ot_vcnt; //ray count, voxel depth count
#else
#define SVO_PRF(x)
#endif

#endif
