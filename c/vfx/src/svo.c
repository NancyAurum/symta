#:slb

inline int32_t __float_as_int(float x) { return *(int32_t*)&x; }

inline float __int_as_float(int32_t x) { return *(float*)&x; }

#LOG2F(x) ((__float_as_int((float)(x)) >> 23) - 127)
#EXP2F(x) __int_as_float(((U32)(x) + 127) << 23)

#IS16BIT_SCALE(scale) ((scale)<scale16)


typedef struct {
  void* nodeStack[OT_MAX_SCALE + 1];
  F32 tmaxStack[OT_MAX_SCALE + 1];
} svo_stack_t;


inline void *stack_read(svo_stack_t *this, int idx, F32 *tmax) {
  *tmax = this->tmaxStack[idx];
  return this->nodeStack[idx];
}

inline void stack_write(svo_stack_t *this, int idx, void* node, F32 tmax)
{
  this->nodeStack[idx] = node;
  this->tmaxStack[idx] = tmax;
}

static svo_stack_t gstack;
int ray_sx;
int ray_sy;

//the leaf mask in the NV algorithm is actually the non-leaf mask
void svo_raycastS(ot_t *this, svo_res_t *res, svo_ray_t ray) {
  seg_t *seg;
  nodes_t *stopo;
  nodes_t *topo = &this->topo;

  svo_stack_t *stack = &gstack;

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
#if 1
      is_filled = 1;
      is_term = 0;
#else
      U32 oct = ((node_t*)node)->octs[oct_index];
      is_filled = oct != OT_EMPTY;
      is_term = IS_PRDF(oct);
#endif
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


