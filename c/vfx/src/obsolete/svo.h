#ifndef SVO_H
#define SVO_H



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
  U32*        node;      // Hit parent voxel (hit_parent)
  int         childIdx;  // Hit child slot index (hit_idx)
  int         stackPtr;  // Hit scale (hit_scale)
} svo_res_t;

typedef struct {
  U32* nodeStack[CAST_STACK_DEPTH + 1];
  F32 tmaxStack[CAST_STACK_DEPTH + 1];
} svo_stack_t;


INLINE U32 *stack_read(svo_stack_t *this, int idx, F32 *tmax) {
  *tmax = this->tmaxStack[idx];
  return this->nodeStack[idx];
}

INLINE void stack_write(svo_stack_t *this, int idx, U32* node, F32 tmax)
{
  this->nodeStack[idx] = node;
  this->tmaxStack[idx] = tmax;
}

INLINE int32_t __float_as_int(float x) { return *(int32_t*)&x; }

INLINE float __int_as_float(int32_t x) { return *(float*)&x; }

#if 1

#if 1
/*INLINE int popc8(uint8_t b) {
  return __builtin_popcount(b);
}*/

//if compiled with -mpopcnt it uses the SSE4 popcnt
#define popc8(b) __builtin_popcount(b)

#else

static const uint8_t popc8_lut[256] = {
#define B2(n) n,     n+1,     n+1,     n+2
#define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
    B6(0), B6(1), B6(1), B6(2)
};
#undef B2
#undef B4
#undef B6
INLINE int popc8(uint8_t b) {
  return popc8_lut[b];
}

#endif


#else
INLINE int popc8(uint8_t b) {
  b = b - ((b >> 1) & 0x55);
  b = (b & 0x33) + ((b >> 2) & 0x33);
  return (((b + (b >> 4)) & 0x0F) * 0x01);
}
#endif

#if 0
INLINE int popc32(uint32_t i) {
   i = i - ((i >> 1) & 0x55555555);
   i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
   return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}
#endif

//if it is commented, then contour field is used to point to attribs
//#define SVO_USE_ALUT 1

//descriptor size
#define SVO_DSZ 2


#define SVO_MESH 0x02000000
#define SVO_VOID 0xFFFFFFFF

/*
typedef struct {
  uint32_t dsc; // descriptor holds flags for each octant: if it is valid
                // and if it is a branch. 
  uint32_t ofs; // points to the kids offsets list.
                // if node has no branches, it points directly into
                // attr list, which holds shading attribs for each leaf.
                // if node has branches, then attrib pointers is in the end of
                // the kids list.
} node;


This SVO format should be at least 2.4 times faster than naive flat SVO.

1. For optimized LOD nodes, kid list can have the offset into node's own attr
2. The upper bits of node descriptors can hold the number of allocated words
   inside the node kids area and attrib link.
   If attrib attrib link is at the end, we can access it with single addition.
3. The same attr can be shared between different nodes. To do that,
   is_instance flag must be introduce into node descriptor
4. Since nodes get moved, their offsets don't can become invalid after a set
   operation. This can be ameliorated by back patching the new offset into
   old node.
5. Top descriptor bits can be used for additional flags, like mesh flags.
6. Attributes would be accessible a bit faster, if the attr offset getsplaced
   first in the list of kids: pkids[popc8(branch_mask)*2] will be replaced
   with pkids[0]. But topology traversal could get a bit slower.
   Considering topology traversal speed to be of bigger importance,
   the attr offset is placed last.
7. For now only the leaves have attribs, which slows down the rendering,
   since we can't unload parts of the model or render only up to the pixel.
8. For fast term splitting we can delay splitting the attrib, until the actual
   painting event.
*/

struct svo_t {
  CXU32s  topo;  //topology: nodes and links into attr (list of unit32_t)
  attrs_t attr;  //attributes
  int c;            //cube edge size (1<<scale)
};


INLINE void svo_init(svo_t *svo, int scale) {
  svo->c = 1<<scale;
  svo->topo.init(128);
  svo->attr.init(128);

  svo->topo.push(0); //empty node
  svo->topo.push(0); //dummy offset
}

INLINE void svo_free(svo_t *svo) {
  svo->topo.free;
  svo->attr.free;
}

INLINE void svo_clear(svo_t *svo, uint32_t color) {
  int c = svo->c;
  svo_free(svo);
  svo_init(svo, 0);
  svo->c = c;
  svo->topo.elts[0] = 0xFF00; //all valid leafs
  svo->topo.elts[1] = svo->attr.used; //no branches: points right into attr

  attr_t value;
  value.color = color;
  value.normal = 0;

  for (int i = 0; i < 8; i++) {
    svo->attr.push(value);
  }
}

#define SVO_NODE_MASKS(node,valid_mask,branch_mask)  \
    uint32_t valid_mask = (*node>>8)&0xFF;           \
    uint32_t branch_mask = *node&0xFF;

#define SVO_NODE_KIDS(pkids,svo,node) \
    uint32_t *pkids = svo->topo.elts+node[1];

INLINE uint32_t svo_attr_ofs(svo_t *svo, uint32_t *node
                            ,uint32_t branch_mask, uint32_t valid_mask) {
  if(!branch_mask) return node[1];
  return svo->topo.elts[node[1] + popc8(branch_mask)*2]; 
}

INLINE attr_t *
svo_attr2(svo_t *svo, uint32_t *node, uint32_t bit
         ,uint32_t branch_mask, uint32_t valid_mask)
{
  uint32_t leaf_mask = ~branch_mask&valid_mask;
  uint32_t oattr = svo_attr_ofs(svo, node, branch_mask, valid_mask);
  return &svo->attr.elts[oattr + popc8(leaf_mask&(bit-1))];
}

INLINE attr_t *svo_t.oct_attr(uint32_t *node, uint32_t oct) {
  return svo_attr2(this, node, 1<<oct, *node&0xFF, (*node>>8)&0xFF);
}

INLINE attr_t *svo_get(svo_t *svo, ivec3 p) {
  uint32_t *node = svo->topo.elts;
  ivec3 axes = {1,2,4};
  int c = svo->c;
  for(;;) {
    SVO_NODE_MASKS(node,valid_mask,branch_mask);
    ivec3 o = {c,c,c};
    ivec3 gt = (p >= o)*-1;
    p -= gt*c;
    int oct = isum(gt*axes);
    uint32_t bit = 1<<oct;
    if (!(valid_mask&bit)) return 0;
    if (!(branch_mask&bit))
      return svo_attr2(svo, node, bit, branch_mask, valid_mask);

    SVO_NODE_KIDS(pkids,svo,node);
    uint32_t kindex = popc8(branch_mask&(bit-1));
    node = pkids + kindex*2;
    c >>= 1;
  }
}

INLINE void svo_set(svo_t *svo, ivec3 p, uint32_t color) {
  uint32_t *node = svo->topo.elts;
  ivec3 axes = {1,2,4};
  attr_t value;
  value.color = color;
  value.normal = 0;
  int c = svo->c;
  for(;;) {
    ivec3 o = {c,c,c};
    ivec3 gt = (p >= o)*-1; //GCC SIMD produce -1 on true
    p -= gt*c;
    c >>= 1;
    int oct = isum(gt*axes);
    uint32_t bit = 1<<oct;
    //fprintf(stderr, "desc: %08x\n", *node);
    SVO_NODE_MASKS(node,valid_mask,branch_mask);
    if (!c) {
      if (color == SVO_VOID) {
        if (valid_mask&bit) {
          uint32_t k = node[1];
          uint32_t i = k+popc8(valid_mask&(bit-1));
          uint32_t n = k+popc8(valid_mask);
          for ( ; i < n-1; i++) { //shift attrs, omitting the erased leaf
            *svo->attr.ref(i) = svo->attr.get(i+1);
          }
          *node ^= bit<<8; //erase it
        }
        return;
      }
      if (valid_mask&bit) { //existing attrib?
        *svo_attr2(svo, node, bit, branch_mask, valid_mask) = value;
      } else { //expand it to include this octant
        uint32_t k = node[1];
        node[1] = svo->attr.used;
        valid_mask |= bit;
        uint32_t bit_i = popc8(valid_mask&(bit-1));
        uint32_t n = popc8(valid_mask);
        for (uint32_t i = 0; i < n; i++) {
          if (i != bit_i) {
            svo->attr.push(svo->attr.get(k++));
          } else {
            svo->attr.push(value);
          }
        }
        *node |= bit<<8; //update valid_mask
      }
      return;
    }

    if (!(valid_mask&bit) || !(branch_mask&bit)) {
      uint32_t leaf_mask = ~branch_mask&valid_mask;
      uint32_t node_ofs = node - svo->topo.elts;
      uint32_t next_desc_ofs;
      uint32_t ndesc = (valid_mask&bit) ? 0xff00 : 0x0000;
      uint32_t oattr;
      if (branch_mask) { //have to copy branches?
        SVO_NODE_KIDS(pkids,svo,node);
        uint32_t prev = pkids - svo->topo.elts;
        node[1] = svo->topo.used;
        for (int i = 0; i < 8; i++) {
          if (branch_mask&(1<<i)) {
            svo->topo.push(svo->topo.get(prev));
            prev++;
            svo->topo.push(svo->topo.get(prev));
            prev++;
          } else if (i == oct) {
            next_desc_ofs = svo->topo.used;
            svo->topo.push(ndesc); //desc
            svo->topo.push(0); //branches/attribs link
          }
        }
        if (leaf_mask) {
          oattr = svo->topo.get(prev);
          svo->topo.push(oattr);
        }
      } else {
        next_desc_ofs = svo->topo.used;
        if (valid_mask) { //has attributes?
          oattr = node[1];
          node[1] = next_desc_ofs;
          svo->topo.push(ndesc);
          svo->topo.push(0);
          svo->topo.push(oattr); //migrate attrib link to the end
        } else {
          node[1] = next_desc_ofs;
          svo->topo.push(ndesc);
          svo->topo.push(0);
        }
      }
      node = svo->topo.ref(node_ofs);
      *node |= (bit<<8) | bit;
      if (valid_mask&bit) { //spliting existing leaf?
        //FIXME: CALC AVG OF KID ATTRIBS
        uint32_t kid_oattr = oattr+popc8(valid_mask&(bit-1));
        attr_t prev_attr = svo->attr.get(kid_oattr);
        svo->topo.elts[next_desc_ofs+1] = svo->attr.used;
        for (int i = 0; i < 8; i++) {
          svo->attr.push(prev_attr);
        }

        uint32_t i = oattr+popc8(valid_mask&(bit-1));
        uint32_t n = oattr+popc8(valid_mask);
        for ( ; i < n-1; i++) { //shift attrs, omitting the erased leaf
          *svo->attr.ref(i) = svo->attr.get(i+1);
        }
      }
    }

    SVO_NODE_KIDS(pkids,svo,node);
    uint32_t kindex = popc8(branch_mask&(bit-1));
    node = pkids + kindex*2;
  }
}


static void svo_list_terms2(int valid
                    ,SVOTerms *list, uint32_t *base, uint32_t *node
                    ,int c, ivec3 pos)
{
  //fprintf(stderr, "node: %d\n", (int)(node-base));
  SVO_NODE_MASKS(node,valid_mask,branch_mask);
  uint32_t *kid = base+node[1];
  int oct;
  for (oct = 0; oct < 8; oct++) {
    vec3 oct_pos = pos + ioct2gt[oct]*c;
    if (branch_mask&(1<<oct)) {
      svo_list_terms2(valid, list, base, kid, c>>1, oct_pos);
      kid += 2;
    } else if (valid == 2                    ||
               ( valid &&  (valid_mask&oct)) ||
               (!valid && !(valid_mask&oct))) {
      uint32_t leaf_index = list.reserve;
      term_t *leaf = list.ref(leaf_index);
      leaf->o = oct_pos;
      leaf->c = c;
      leaf->node = node - base;
      leaf->oct = oct;
    }
  }
}

#define svo_e(svo) ((svo)->topo.elts)

#define svo_leaf_list(svo, list) \
  SVOTerms list;                \
  list.init(128);                \
  svo_list_terms2(1, &list, svo_e(svo), svo_e(svo), svo->c, (ivec3){0,0,0});

#define svo_empty_list(svo, list) \
  SVOTerms list;                \
  list.init(128);                \
  svo_list_terms2(0, &list, svo_e(svo), svo_e(svo), svo->c, (ivec3){0,0,0});

#define svo_term_list(svo, list) \
  SVOTerms list;                \
  list.init(128);                \
  svo_list_terms2(2, &list, svo_e(svo), svo_e(svo), svo->c, (ivec3){0,0,0});

static attr_t *svo_term_attr(svo_t *svo, term_t *leaf) {
  uint32_t *node = svo->topo.elts+leaf->node;
  SVO_NODE_MASKS(node,valid_mask,branch_mask);
  return svo_attr2(svo, node, 1<<leaf->oct, branch_mask, valid_mask);
}


static void svo_print_term_list(svo_t *svo, SVOTerms *list) {
  for (int i = 0; i < list->used; i++) {
    term_t *t = &list->elts[i];
    //fprintf(stderr, "i=%x\n", t->node);
    uint32_t *node = svo->topo.elts + t->node;
    SVO_NODE_MASKS(node,valid_mask,branch_mask);
    uint32_t leaf_mask = ~branch_mask&valid_mask;
    uint32_t bit = 1<<t->oct;
    int exist = valid_mask&bit ? 1 : 0;
    uint32_t color = 0xFFFFFFFF;
    if (leaf_mask&bit) {
      color = svo_term_attr(svo, t)->color;
    }
    fprintf(stderr, "(%d,%d,%d):%d:%06x+%d:%s:color=#%08x\n"
           ,t->o.x, t->o.y, t->o.z, t->c
           ,t->node, t->oct
           ,exist ? "exist" : "empty"
           ,color);
  }
}

static void svo_print_terms(svo_t *svo) {
  svo_term_list(svo, list);
  svo_print_term_list(svo, &list);
}

void svo_t.raycast2(svo_res_t *res, svo_stack_t *stack, svo_ray_t ray) {
  svo_t *svo = this;
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
  uint32_t*   node        = svo->topo.elts;
  uint32_t descriptor     = 0; //invalid until fetched
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
    //DBG("scale: %d; child: %d; pos: %f,%f,%f\n",scale,idx,pos.x,pos.y,pos.z);

    if (!descriptor) {
      descriptor = *node;
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
        SVO_NODE_KIDS(pkids,svo,node);
        node = pkids + popc8(child_masks&0x7F)*SVO_DSZ;

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
      node = (uint32_t*)stack_read(stack, scale, &t_max);

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

static svo_stack_t stack;
static int ray_sx,ray_sy;

INLINE void svo_t.raycast(rt_t *RESTRICT rt) {
  rt->t = INFINITY;
  rt->attr = 0;
  rt->ird = 1.0f/(rt->rd + (vec3){FLT_MIN,FLT_MIN,FLT_MIN});

  //if (!ray_aabb_hit_test1(rt->ro, rt->ird, rt->box)) return; //miss scene box

  //vec3 box = (vec3){size,size,size}; //rt->box;
  vec2 hit = ray_box_hit_test(rt->ro, rt->rd, (vec3){0,0,0}, rt->box);
  if (hit.x == INFINITY) return;

  //if (ray_sx == 60 && ray_sy == 60) bug = 1;
  //if (ray_sx == 40 && ray_sy == 70) bug = 1;
  //if (ray_sx == 40 && ray_sy == 55) bug = 1;


  //The octree resides at (1,1,1) and has size 1.
  //when ray origin is out of the tree, it will underflow the stack
  //the leaf mask is actually non-lead mask
  DBG("----------------------------\n");
  svo_t *svo = (svo_t*)rt->svo;
  float size = svo->c*2.0f;
  svo_ray_t ray;
  ray.orig = rt->ro; //+ hit.x*rt->rd;
  ray.orig_sz = 0.0;
  ray.dir = rt->rd*size;
  ray.dir_sz = 0.01;
  //VDBG("ro",ray.orig);
  ray.orig /= size;
  //DBG("size: %f\n", size);
  ray.orig += (vec3){1.0f,1.0f,1.0f};
  svo_res_t res;
  svo.raycast2(&res, &stack, ray);
  if (res.t > 1.0f) { // No hit => sky.
    return;
  }
  //fprintf(stderr, "hit!\n");

  rt->t = res.t*size*size;

  //rt->t = hit.x + res.t*size*size;//double size, since we have divided ray.dir
  //rt->t = hit.x + distance(res.pos,ray.orig) * size;
#if 0
  rt->voxel = 0xFF0000;
#else
  uint32_t *node = (uint32_t*)res.node;
  rt->attr = svo.oct_attr(node, res.childIdx);
  if (!rt->attr) {
    static attr_t attr;
    attr.color = 0xFFFFFF;
    attr.normal = NIL_NORMAL;
    rt->attr = &attr;
  }
#endif
}


//list non-void leaf terms
INLINE SVOTerms *svo_t.terms {
  auto svo = this;
  auto terms = new(SVOTerms);
  svo_list_terms2(1, terms, svo_e(svo), svo_e(svo), svo->c, (ivec3){0,0,0});
  return terms;
}

INLINE attr_t *svo_t.term_attr(term_t *t) {
  return svo_term_attr(this, t);
}

INLINE SVOTerms *svo_t.mesh_terms {
  auto svo = this;
  auto terms = new(SVOTerms);
  svo_list_terms2(1, terms, svo_e(svo), svo_e(svo), svo->c, (ivec3){0,0,0});
  auto mterms = new(SVOTerms);
  foreach(t,terms) {
    if (svo.term_attr(t)->color&SVO_MESH) mterms.push(*t);
  }
  delete(terms);
  return mterms;
}

INLINE uint32_t svo_t.get(uint32_t x, uint32_t y, uint32_t z) {
  attr_t *attr = svo_get(this, (ivec3){x,y,z});
  if (!attr) return SVO_VOID;
  uint32_t c = attr->color;
  return c==SVO_VOID ? 0 : c&0xFFFFFF;
}

INLINE svo_t *svo_t.set(uint32_t x, uint32_t y, uint32_t z, uint32_t color) {
  if (!color) color = SVO_VOID;
  svo_set(this, (ivec3){x,y,z}, color);
  return this;
}

INLINE attr_t *svo_t.getf(vec3 p) {
  return svo_get(this, (ivec3){(int)p.x,(int)p.y,(int)p.z});
}

INLINE uint32_t svo_t.exists(term_t *t) {
  return svo_term_attr(this, t) != 0;
}

INLINE uint32_t svo_t.term_color(term_t *t) {
  return svo_term_attr(this, t)->color;
}

INLINE void svo_t.term_set(term_t *t, uint32_t color) {
  svo_t *svo = this;
  if (color != SVO_VOID) {
    svo_term_attr(this, t)->color = color;
    return;
  }
  uint32_t *node = svo->topo.elts+t->node;
  SVO_NODE_MASKS(node,valid_mask,branch_mask);
  uint32_t leaf_mask = ~branch_mask&valid_mask;
  uint32_t bit = 1<<t->oct;
  uint32_t k = svo_attr_ofs(svo, node, branch_mask, valid_mask);
  uint32_t i = k+popc8(leaf_mask&(bit-1));
  uint32_t n = k+popc8(leaf_mask);
  for ( ; i < n-1; i++) { //shift attrs, omitting the erased leaf
    *svo->attr.ref(i) = svo->attr.get(i+1);
  }
  node[0] ^= bit<<8; //erase it
}

INLINE void svo_t.term_set_mesh(term_t *t, uint32_t color) {
  svo_term_attr(this, t)->color = color|SVO_MESH;
}

INLINE void svo_t.term_erase(term_t *t) {
  this.term_set(t, SVO_VOID);
}

//FIXME: the algorithm would be far more efficient,
//       if we first mark the terms for splitting, and split only
//       i.e. store the terms to split in a special list.
//       when the current set is completely consumed
INLINE void svo_t.term_split(term_t *t, SVOTerms *terms) {
  svo_t *svo = this;
  uint32_t *node = svo->topo.elts+t->node;
  SVO_NODE_MASKS(node,valid_mask,branch_mask);
  uint32_t leaf_mask = ~branch_mask&valid_mask;
  // shift erase the attr for the splitted oct
  uint32_t toct = t->oct;
  uint32_t bit = 1<<toct;
  uint32_t k = svo_attr_ofs(svo, node, branch_mask, valid_mask);
  uint32_t i = k+popc8(leaf_mask&(bit-1));
  uint32_t n = k+popc8(leaf_mask);
  attr_t attr = svo->attr.elts[i]; //save the splitted attr
  for ( ; i < n-1; i++) { //shift attrs, omitting the erased leaf
    *svo->attr.ref(i) = svo->attr.get(i+1);
  }
  int c = t->c;
  ivec3 pos = t->o;
  SVO_NODE_KIDS(pkids,svo,node);
  uint32_t kido = pkids - svo->topo.elts; //prev kids offset, since topo grows
  node[0] |= bit; //turn this oct into a branch
  node[1] = svo->topo.used; //splitting requires new topology
  int eoct;
  for (eoct = 0; eoct < 8; eoct++) {
    if (branch_mask&(1<<eoct)) {
      //copy the existing branch
      svo->topo.push(svo->topo.get(kido));
      kido++;
      svo->topo.push(svo->topo.get(kido));
      kido++;
    } else if (eoct == toct) { //the oct we are splitting?
      //create the new branch
      uint32_t node_off = svo->topo.used;
      svo->topo.push(0xFF00); //all kids are valid non-branches
      svo->topo.push(svo->attr.used); //attr pointer
      for (int oct = 0; oct < 8; oct++) {
        svo->attr.push(attr);
        uint32_t leaf_index = terms.reserve;
        vec3 oct_pos = pos + ioct2gt[oct]*c;
        term_t *leaf = terms.ref(leaf_index);
        leaf->o = oct_pos;
        leaf->c = c;
        leaf->node = node_off;
        leaf->oct = oct;
      }
    }
  }
}

INLINE void svo_t.optimize {
}

INLINE void svo_t.realloc {
#if 0
  CXU32s ntopo = CXLstInit;
  attrs_t nattr = CXLstInit;
  this.realloc_r(&ntopo, &nattr, 0);
  this->topo.free; 
  this->topt = ntree;
  this->attr.free; 
  this->attr = nattr;
#endif
}

INLINE void svo_t.clear(uint32_t color) {
  svo_clear(this, color);
}

INLINE void svo_t.init(void *slb, uint32_t c) {
  svo_init(this, c); 
}

INLINE void svo_t.free {
  svo_free(this);
}

INLINE void svo_t.clone(svo_t *osvo) {
  this->c = osvo->c;
  this->topo.copy(&osvo->topo);
  this->attr.copy(&osvo->attr);
}

INLINE void svo_t.clear_interior(uint32_t clear_color) {
  //this should clear non-mesh pixel into color
}


INLINE void svo_t.file_save(FILE *out, slab_t *slb) {
  uint32_t c = this->c;
  fwrite(&c, 1, 4, out);
  this->topo.file_save(out);
  this->attr.file_save(out);
}

INLINE void svo_t.file_load(FILE *in, slab_t *slb) {
  uint32_t c;
  fread(&c, 1, 4, in);
  this->c = c;
  this->topo.file_load(in);
  this->attr.file_load(in);
}

#endif //SVO_H