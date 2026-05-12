/*

Copyright (c) 2009-2011, NVIDIA Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of NVIDIA Corporation nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


Modified by Nash Gold.

*/

#ifndef SVO_H
#define SVO_H

typedef unsigned char       U8;
typedef unsigned short      U16;
typedef unsigned int        U32;
typedef signed char         S8;
typedef signed short        S16;
typedef signed int          S32;
typedef float               F32;
typedef double              F64;

typedef uint64_t            U64;
typedef int64_t             S64;



//------------------------------------------------------------------------

#define FW_U32_MAX          (0xFFFFFFFFu)
#define FW_S32_MIN          (~0x7FFFFFFF)
#define FW_S32_MAX          (0x7FFFFFFF)
#define FW_U64_MAX          ((U64)(S64)-1)
#define FW_S64_MIN          ((S64)-1 << 63)
#define FW_S64_MAX          (~((S64)-1 << 63))
#define FW_F32_MIN          (1.175494351e-38f)
#define FW_F32_MAX          (3.402823466e+38f)
#define FW_F64_MIN          (2.2250738585072014e-308)
#define FW_F64_MAX          (1.7976931348623158e+308)
#define FW_PI               (3.14159265358979323846f)

#define CAST_STACK_DEPTH        23
#define MAX_RAYCAST_ITERATIONS  10000

typedef vec3 float3;
typedef ivec2 int2;

#define make_float3(x,y,z) ((vec3){(float)(x),(float)(y),(float)(z)})
#define make_int2(x,y) ((ivec2){(int)(x),(int)(y)})

typedef struct {
  float3      orig;
  float       orig_sz; // LOD at ray origin (ray_size_bias)
  float3      dir;
  float       dir_sz;  // LOD increase along ray (ray_size_coef)
} Ray;

typedef struct {
  float       t;         // Hit t-value (hit_t)
  float3      pos;       // Hit position (hit_pos)
  int         iter;      // Number of iterations
  S32*        node;      // Hit parent voxel (hit_parent)
  int         childIdx;  // Hit child slot index (hit_idx)
  int         stackPtr;  // Hit scale (hit_scale)
} CastResult;

typedef struct {
    S32*            nodeStack[CAST_STACK_DEPTH + 1];
    F32             tmaxStack[CAST_STACK_DEPTH + 1];
} CastStack;


static INLINE S32 *stack_read(CastStack *this, int idx, F32 *tmax) {
  *tmax = this->tmaxStack[idx];
  return this->nodeStack[idx];
}

static INLINE void stack_write(CastStack *this, int idx, S32* node, F32 tmax) {
  this->nodeStack[idx] = node;
  this->tmaxStack[idx] = tmax;
}

#define PERF_COUNTER_LIST(X) \
    X(Instructions) \
    X(Iterations) \
    X(Intersect) \
    X(Push) \
    X(PushStore) \
    X(Advance) \
    X(Pop) \
    X(GlobalAccesses) \
    X(GlobalBytes) \
    X(GlobalTransactions) \
    X(LocalAccesses) \
    X(LocalBytes) \
    X(LocalTransactions)

enum PerfCounter
{
#define X(NAME) PerfCounter_ ## NAME,
    PERF_COUNTER_LIST(X)
#undef X
    PerfCounter_Max
};


static int PerfCounters[PerfCounter_Max];

#define updateCounter(counter,amount) \
  do {PerfCounters[counter] += (amount);} while (0)
#define updateCountersForLocalAccess(id,address)
#define updateCountersForGlobalAccess(id,address)

static INLINE int __float_as_int(float x) {  return *(int*)&x; }

static INLINE float __int_as_float(int x) { return *(float*)&x; }

static INLINE int popc8(uint8_t b) {
  b = b - ((b >> 1) & 0x55);
  b = (b & 0x33) + ((b >> 2) & 0x33);
  return (((b + (b >> 4)) & 0x0F) * 0x01);
}

#if 0
void castRay(CastResult *res, CastStack *stack, int *rootNode, Ray ray) {
  const float epsilon = exp2f(-CAST_STACK_DEPTH);
  float ray_orig_sz = ray.orig_sz;
  int iter = 0;

  // Get rid of small ray direction components to avoid division by zero.

  if (fabsf(ray.dir.x) < epsilon) ray.dir.x = copysignf(epsilon, ray.dir.x);
  if (fabsf(ray.dir.y) < epsilon) ray.dir.y = copysignf(epsilon, ray.dir.y);
  if (fabsf(ray.dir.z) < epsilon) ray.dir.z = copysignf(epsilon, ray.dir.z);
  
  DBG("ray.dir: %.10f,%.10f,%10f\n", ray.dir.x, ray.dir.y, ray.dir.z);
  DBG("ray.orig: %.10f,%.10f,%10f\n", ray.orig.x, ray.orig.y, ray.orig.z);

  // Precompute the coefficients of tx(x), ty(y), and tz(z).
  // The octree is assumed to reside at coordinates [1, 2].

  float tx_coef = 1.0f / -fabs(ray.dir.x);
  float ty_coef = 1.0f / -fabs(ray.dir.y);
  float tz_coef = 1.0f / -fabs(ray.dir.z);

  float tx_bias = tx_coef * ray.orig.x;
  float ty_bias = ty_coef * ray.orig.y;
  float tz_bias = tz_coef * ray.orig.z;

  //DBG("t_coef: %f,%f,%f\n", tx_coef, ty_coef, tz_coef);
  //DBG("t_bias: %f,%f,%f\n", tx_bias, ty_bias, tz_bias);


  // Select octant mask to mirror the coordinate system so
  // that ray direction is negative along each axis.

  int octant_mask = 7;
  if (ray.dir.x > 0.0f) octant_mask ^= 1, tx_bias = 3.0f * tx_coef - tx_bias;
  if (ray.dir.y > 0.0f) octant_mask ^= 2, ty_bias = 3.0f * ty_coef - ty_bias;
  if (ray.dir.z > 0.0f) octant_mask ^= 4, tz_bias = 3.0f * tz_coef - tz_bias;

  //DBG("t_bias2: %f,%f,%f\n", tx_bias, ty_bias, tz_bias);

  //DBG("octant_mask: %x\n", octant_mask);

  // Initialize the active span of t-values.

  float t_min = fmaxf(fmaxf(2.0f * tx_coef - tx_bias, 2.0f*ty_coef - ty_bias),
         2.0f * tz_coef - tz_bias);
  float t_max = fminf(fminf(tx_coef - tx_bias, ty_coef - ty_bias),
         tz_coef - tz_bias);
  float h = t_max;
  t_min = fmaxf(t_min, 0.0f);
  t_max = fminf(t_max, 1.0f);

  // Initialize the current voxel to the first child of the root.

  int*   parent           = rootNode;
  int2   child_descriptor = make_int2(0, 0); // invalid until fetched
  int    idx              = 0;
  float3 pos              = make_float3(1.0f, 1.0f, 1.0f);
  int    scale            = CAST_STACK_DEPTH - 1;
  float  scale_exp2       = 0.5f; // exp2f(scale - s_max)

  if (1.5f * tx_coef - tx_bias > t_min) idx ^= 1, pos.x = 1.5f;
  if (1.5f * ty_coef - ty_bias > t_min) idx ^= 2, pos.y = 1.5f;
  if (1.5f * tz_coef - tz_bias > t_min) idx ^= 4, pos.z = 1.5f;


  //DBG("pos: %f,%f,%f\n", pos.x, pos.y, pos.z);

  // Traverse voxels along the ray as long as the current voxel
  // stays within the octree.

  while (scale < CAST_STACK_DEPTH)
  {
    DBG("scale: %d\n", scale);
    
    updateCounter(PerfCounter_Iterations,1);
    updateCounter(PerfCounter_Instructions, 15 - 1);
#ifndef KERNEL_RAYCAST_PERF
    iter++;
#if (MAX_RAYCAST_ITERATIONS > 0)
    if (iter > MAX_RAYCAST_ITERATIONS)
      break;
#endif
#endif

    // Fetch child descriptor unless it is already valid.

    if (child_descriptor.x == 0)
    {
      child_descriptor = *(int2*)parent;
      updateCountersForGlobalAccess(3, parent);
    }

    //DBG("child_descriptor: %x\n", child_descriptor.x);

    // Determine maximum t-value of the cube by evaluating
    // tx(), ty(), and tz() at its corner.

    float tx_corner = pos.x * tx_coef - tx_bias;
    float ty_corner = pos.y * ty_coef - ty_bias;
    float tz_corner = pos.z * tz_coef - tz_bias;
    float tc_max = fminf(fminf(tx_corner, ty_corner), tz_corner);

    //DBG("t_corner: %f, %f, %f\n",tx_corner, ty_corner, tz_corner);

    // Process voxel if the corresponding bit in valid mask is set
    // and the active t-span is non-empty.

    int child_shift = // permute child slots based on the mirroring
          idx ^ octant_mask;
    int child_masks = child_descriptor.x << child_shift;
    //DBG("child_shift: %d\n", child_shift);
    //DBG("valid_child: %d\n", (child_masks & 0x8000) != 0);
    //DBG("tmin=%f, tmax=%f\n", t_min, t_max);
    if ((child_masks & 0x8000) != 0 && t_min <= t_max)
    {
      // Terminate if the voxel is small enough.

      updateCounter(PerfCounter_Instructions, 3);
      if (tc_max * ray.dir_sz + ray_orig_sz >= scale_exp2) {
        DBG("small_enough break: %f,%f\n", tc_max,scale_exp2);
        break; // at t_min
      }

      // INTERSECT
      // Intersect active t-span with the cube and evaluate
      // tx(), ty(), and tz() at the center of the voxel.

      updateCounter(PerfCounter_Intersect,1);
      updateCounter(PerfCounter_Instructions, 9 - 1);
      float tv_max = fminf(t_max, tc_max);
      float half = scale_exp2 * 0.5f;
      float tx_center = half * tx_coef + tx_corner;
      float ty_center = half * ty_coef + ty_corner;
      float tz_center = half * tz_coef + tz_corner;

      // Intersect with contour if the corresponding bit in contour mask is set.

      int contour_mask = child_descriptor.y << child_shift;
#ifndef ENABLE_CONTOURS
      contour_mask = 0;
#endif

      if ((contour_mask & 0x80) != 0)
      {
        updateCounter(PerfCounter_Instructions, 35 - 9);

        // contour pointer
        int ofs  = (unsigned int)child_descriptor.y >> 8;

        // contour value
        int value  = parent[ofs + popc8(contour_mask & 0x7F)];
        updateCountersForGlobalAccess(2,
               &parent[ofs + popc8(contour_mask & 0x7F)]);

        // thickness
        float cthick = (float)(unsigned int)value * scale_exp2 * 0.75f;

        // position
        float cpos = (float)(value << 7) * scale_exp2 * 1.5f;

    
        float cdirx  = (float)(value << 14) * ray.dir.x; // nx
        float cdiry  = (float)(value << 20) * ray.dir.y; // ny
        float cdirz  = (float)(value << 26) * ray.dir.z; // nz
        float tcoef  = 1.0f / (cdirx + cdiry + cdirz);
        float tavg = tx_center*cdirx + ty_center*cdiry
             + tz_center*cdirz + cpos;
        float tdiff  = cthick * tcoef;

        // Override t_min with tv_min.
        t_min  = fmaxf(t_min,  tcoef * tavg - fabsf(tdiff));
        tv_max = fminf(tv_max, tcoef * tavg + fabsf(tdiff));
      }

      // Descend to the first child if the resulting t-span is non-empty.

      updateCounter(PerfCounter_Instructions, 2);
      if (t_min <= tv_max)
      {
        // Terminate if the corresponding bit in the non-leaf mask is not set.

        updateCounter(PerfCounter_Instructions, 2);
        if ((child_masks & 0x0080) == 0)
          break; // at t_min (overridden with tv_min).

        // PUSH
        // Write current parent to the stack.

        updateCounter(PerfCounter_Push, 1);
        updateCounter(PerfCounter_Instructions, 31 - 6);
#ifndef DISABLE_PUSH_OPTIMIZATION
        if (tc_max < h)
#endif
        {
          updateCounter(PerfCounter_PushStore, 1);
          DBG("push: %d\n", scale);
          stack_write(stack, scale, parent, t_max);
          updateCountersForLocalAccess(3, scale);
        }
        h = tc_max;

        // Find child descriptor corresponding to the current voxel.

        // child pointer
        int ofs = (unsigned int)child_descriptor.x >> 17;
        if ((child_descriptor.x & 0x10000) != 0) // far
        {
          ofs = parent[ofs * 2]; // far pointer
          updateCountersForGlobalAccess(2, &parent[ofs * 2]);
        }
        ofs += popc8(child_masks & 0x7F);
        parent += ofs * 2;

        // Select child voxel that the ray enters first.

        idx = 0;
        scale--;
        scale_exp2 = half;

        if (tx_center > t_min) idx ^= 1, pos.x += scale_exp2;
        if (ty_center > t_min) idx ^= 2, pos.y += scale_exp2;
        if (tz_center > t_min) idx ^= 4, pos.z += scale_exp2;

        // Update active t-span and invalidate cached child descriptor.

        t_max = tv_max;
        child_descriptor.x = 0;
        continue;
      }
    }

    // ADVANCE
    // Step along the ray.

    updateCounter(PerfCounter_Advance,1);
    updateCounter(PerfCounter_Instructions, 14 - 2);
    int step_mask = 0;
    if (tx_corner <= tc_max) step_mask ^= 1, pos.x -= scale_exp2;
    if (ty_corner <= tc_max) step_mask ^= 2, pos.y -= scale_exp2;
    if (tz_corner <= tc_max) step_mask ^= 4, pos.z -= scale_exp2;

    //DBG("pos: %f,%f,%f\n", pos.x, pos.y, pos.z);

    // Update active t-span and flip bits of the child slot index.

    t_min = tc_max;
    idx ^= step_mask;

    DBG("proceed_pop: %d\n", (idx & step_mask) != 0);

    // Proceed with pop if the bit flips disagree with the ray direction.
    if ((idx & step_mask) != 0)
    {
      // POP
      // Find the highest differing bit between the two positions.

      updateCounter(PerfCounter_Pop,1);
      updateCounter(PerfCounter_Instructions, 38 - 4);
      unsigned int differing_bits = 0;
      if ((step_mask & 1) != 0) differing_bits |= __float_as_int(pos.x) ^
                 __float_as_int(pos.x + scale_exp2);
      if ((step_mask & 2) != 0) differing_bits |= __float_as_int(pos.y) ^
                 __float_as_int(pos.y + scale_exp2);
      if ((step_mask & 4) != 0) differing_bits |= __float_as_int(pos.z) ^
                 __float_as_int(pos.z + scale_exp2);

      // position of the highest bit
      scale = (__float_as_int((float)differing_bits) >> 23) - 127;
      scale_exp2 = __int_as_float((scale - CAST_STACK_DEPTH + 127)
                  << 23); // exp2f(scale - s_max)

      // Restore parent voxel from the stack.

      DBG("pop: %d\n", scale);

      parent = stack_read(stack, scale, &t_max);
      updateCountersForLocalAccess(3, scale);

      // Round cube position and extract child slot index.

      int shx = __float_as_int(pos.x) >> scale;
      int shy = __float_as_int(pos.y) >> scale;
      int shz = __float_as_int(pos.z) >> scale;
      pos.x = __int_as_float(shx << scale);
      pos.y = __int_as_float(shy << scale);
      pos.z = __int_as_float(shz << scale);
      idx  = (shx & 1) | ((shy & 1) << 1) | ((shz & 1) << 2);

      // Prevent same parent from being stored again and invalidate cached child descriptor.

      h = 0.0f;
      child_descriptor.x = 0;
    }
    updateCounter(PerfCounter_Instructions, 2);
  }

  // Indicate miss if we are outside the octree.

#if (MAX_RAYCAST_ITERATIONS > 0)
  if (scale >= CAST_STACK_DEPTH || iter > MAX_RAYCAST_ITERATIONS)
#else
  if (scale >= CAST_STACK_DEPTH)
#endif
  {
    t_min = 2.0f;
  }

  // Undo mirroring of the coordinate system.

  if ((octant_mask & 1) == 0) pos.x = 3.0f - scale_exp2 - pos.x;
  if ((octant_mask & 2) == 0) pos.y = 3.0f - scale_exp2 - pos.y;
  if ((octant_mask & 4) == 0) pos.z = 3.0f - scale_exp2 - pos.z;

  // Output results.

  res->t = t_min;
  res->iter = iter;
  res->pos.x = fminf(fmaxf(ray.orig.x + t_min * ray.dir.x, pos.x + epsilon),
         pos.x + scale_exp2 - epsilon);
  res->pos.y = fminf(fmaxf(ray.orig.y + t_min * ray.dir.y, pos.y + epsilon),
         pos.y + scale_exp2 - epsilon);
  res->pos.z = fminf(fmaxf(ray.orig.z + t_min * ray.dir.z, pos.z + epsilon),
         pos.z + scale_exp2 - epsilon);
  res->node = parent;
  res->childIdx = idx ^ octant_mask ^ 7;
  res->stackPtr = scale;
}

#else

void castRay(CastResult *res, CastStack *stack, int *rootNode, Ray ray) {
  const float epsilon = exp2f(-CAST_STACK_DEPTH);

  // Get rid of small ray direction components to avoid division by zero.
  if (fabsf(ray.dir.x) < epsilon) ray.dir.x = copysignf(epsilon, ray.dir.x);
  if (fabsf(ray.dir.y) < epsilon) ray.dir.y = copysignf(epsilon, ray.dir.y);
  if (fabsf(ray.dir.z) < epsilon) ray.dir.z = copysignf(epsilon, ray.dir.z);
  
  //DBG("ray.dir: %.10f,%.10f,%10f\n", ray.dir.x, ray.dir.y, ray.dir.z);
  //DBG("ray.orig: %.10f,%.10f,%10f\n", ray.orig.x, ray.orig.y, ray.orig.z);

  // Precompute the coefficients of tx(x), ty(y), and tz(z).
  // The octree is assumed to reside at coordinates [1, 2].
  float tx_coef = 1.0f / -fabs(ray.dir.x);
  float ty_coef = 1.0f / -fabs(ray.dir.y);
  float tz_coef = 1.0f / -fabs(ray.dir.z);
  float tx_bias = tx_coef * ray.orig.x;
  float ty_bias = ty_coef * ray.orig.y;
  float tz_bias = tz_coef * ray.orig.z;

  //DBG("t_coef: %f,%f,%f\n", tx_coef, ty_coef, tz_coef);
  //DBG("t_bias: %f,%f,%f\n", tx_bias, ty_bias, tz_bias);


  // Select octant mask to mirror the coordinate system so
  // that ray direction is negative along each axis.
  int octant_mask = 7;
  if (ray.dir.x > 0.0f) octant_mask ^= 1, tx_bias = 3.0f * tx_coef - tx_bias;
  if (ray.dir.y > 0.0f) octant_mask ^= 2, ty_bias = 3.0f * ty_coef - ty_bias;
  if (ray.dir.z > 0.0f) octant_mask ^= 4, tz_bias = 3.0f * tz_coef - tz_bias;

  //DBG("t_bias2: %f,%f,%f\n", tx_bias, ty_bias, tz_bias);

  DBG("octant_mask: %x\n", octant_mask);

  // Initialize the active span of t-values.
  float t_min = fmaxf(fmaxf(2.0f * tx_coef - tx_bias, 2.0f*ty_coef - ty_bias),
         2.0f * tz_coef - tz_bias);
  float t_max = fminf(fminf(tx_coef-tx_bias, ty_coef-ty_bias), tz_coef-tz_bias);
  float h = t_max;
  t_min = fmaxf(t_min, 0.0f);
  t_max = fminf(t_max, 1.0f);

  // Initialize the current voxel to the first child of the root.
  int*   parent           = rootNode;
  int2   child_descriptor = make_int2(0, 0); // invalid until fetched
  int    idx              = 0; //child index
  float3 pos              = make_float3(1.0f, 1.0f, 1.0f);
  int    scale            = CAST_STACK_DEPTH - 1;
  float  scale_exp2       = 0.5f; // exp2f(scale - s_max)

  if (1.5f*tx_coef - tx_bias > t_min) idx ^= 1, pos.x = 1.5f;
  if (1.5f*ty_coef - ty_bias > t_min) idx ^= 2, pos.y = 1.5f;
  if (1.5f*tz_coef - tz_bias > t_min) idx ^= 4, pos.z = 1.5f;

  // Traverse voxels along the ray as long as the current voxel
  // stays within the octree.
  while (scale < CAST_STACK_DEPTH) {
    DBG("scale: %d; child: %d; pos: %f,%f,%f\n",scale,idx,pos.x,pos.y,pos.z);

    // Fetch child descriptor unless it is already valid.
    if (child_descriptor.x == 0) child_descriptor = *(int2*)parent;

    //DBG("child_descriptor: %x\n", child_descriptor.x);

    // Determine maximum t-value of the cube by evaluating
    // tx(), ty(), and tz() at its corner.
    float tx_corner = pos.x * tx_coef - tx_bias;
    float ty_corner = pos.y * ty_coef - ty_bias;
    float tz_corner = pos.z * tz_coef - tz_bias;
    float tc_max = fminf(fminf(tx_corner, ty_corner), tz_corner);

    //DBG("t_corner: %f, %f, %f\n",tx_corner, ty_corner, tz_corner);


    // permute child slots based on the mirroring
    int child_shift = idx ^ octant_mask;
    int child_masks = child_descriptor.x << child_shift;
    //DBG("child_shift: %d\n", child_shift);
    //DBG("valid_child: %d\n", (child_masks & 0x8000) != 0);
    //DBG("tmin=%f, tmax=%f\n", t_min, t_max);

    // Process voxel if the corresponding bit in valid mask is set
    // and the active t-span is non-empty.
    if ((child_masks&0x8000) && t_min <= t_max) {
      // Terminate if the voxel is small enough.
      if (tc_max*ray.dir_sz + ray.orig_sz >= scale_exp2) {
        DBG("small_enough_break: %f,%f\n", tc_max,scale_exp2);
        break; // at t_min
      }

      // INTERSECT
      // Intersect active t-span with the cube and evaluate
      // tx(), ty(), and tz() at the center of the voxel.
      float tv_max = fminf(t_max, tc_max);

      // Descend to the first child if the resulting t-span is non-empty.
      if (t_min <= tv_max) {
        // Terminate if the corresponding bit in the non-leaf mask is not set.
        DBG("child_masks: %08x == %08x\n", child_masks, 0xFFFF<<6);
        DBG("leaf_hit: %08x\n",  (0xFFFF<<6)&0x0080);
        if (!(child_masks & 0x0080)) {
          DBG("LEAF_HIT\n");
          break; //at t_min (overridden with tv_min)
        }

        // Write current parent to the stack.
        if (tc_max < h) {
          //DBG("push: %d\n", scale);
          stack_write(stack, scale, parent, t_max);
        }
        h = tc_max;

        // Find child descriptor corresponding to the current voxel.
        int ofs = (unsigned int)child_descriptor.x >> 17; // child pointer
        if (child_descriptor.x&0x10000) ofs = parent[ofs*2]; // far
        ofs += popc8(child_masks & 0x7F);
        parent += ofs*2;

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
        child_descriptor.x = 0;
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
      unsigned int differing_bits = 0;
      if ((step_mask & 1) != 0) differing_bits |= __float_as_int(pos.x) ^
                 __float_as_int(pos.x + scale_exp2);
      if ((step_mask & 2) != 0) differing_bits |= __float_as_int(pos.y) ^
                 __float_as_int(pos.y + scale_exp2);
      if ((step_mask & 4) != 0) differing_bits |= __float_as_int(pos.z) ^
                 __float_as_int(pos.z + scale_exp2);

      // position of the highest bit
      scale = (__float_as_int((float)differing_bits) >> 23) - 127;
      scale_exp2 = __int_as_float((scale - CAST_STACK_DEPTH + 127)
                  << 23); // exp2f(scale - s_max)

      // Restore parent voxel from the stack.
      //DBG("pop: %d\n", scale);
      parent = stack_read(stack, scale, &t_max);

      // Round cube position and extract child slot index.
      int shx = __float_as_int(pos.x) >> scale;
      int shy = __float_as_int(pos.y) >> scale;
      int shz = __float_as_int(pos.z) >> scale;
      pos.x = __int_as_float(shx << scale);
      pos.y = __int_as_float(shy << scale);
      pos.z = __int_as_float(shz << scale);
      idx  = (shx & 1) | ((shy & 1) << 1) | ((shz & 1) << 2);

      // Prevent same parent from being stored again and invalidate cached
      // child descriptor.
      h = 0.0f;
      child_descriptor.x = 0;
    }
  }

  // Indicate miss if we are outside the octree.
  if (scale >= CAST_STACK_DEPTH) t_min = 2.0f;

  // Undo mirroring of the coordinate system.
  if ((octant_mask & 1) == 0) pos.x = 3.0f - scale_exp2 - pos.x;
  if ((octant_mask & 2) == 0) pos.y = 3.0f - scale_exp2 - pos.y;
  if ((octant_mask & 4) == 0) pos.z = 3.0f - scale_exp2 - pos.z;

  // Output results.
  res->t = t_min;
  res->pos.x = fminf(fmaxf(ray.orig.x + t_min * ray.dir.x, pos.x + epsilon),
         pos.x + scale_exp2 - epsilon);
  res->pos.y = fminf(fmaxf(ray.orig.y + t_min * ray.dir.y, pos.y + epsilon),
         pos.y + scale_exp2 - epsilon);
  res->pos.z = fminf(fmaxf(ray.orig.z + t_min * ray.dir.z, pos.z + epsilon),
         pos.z + scale_exp2 - epsilon);
  res->node = parent;
  res->childIdx = idx ^ octant_mask ^ 7;
  res->stackPtr = scale;
}

#endif

typedef struct {
  uint32_t *base;
  uint32_t ptr;
} nvsvo_out_t;

#define OB(oct) (1<<(oct))


static
void ot2nvsvo_r(nvsvo_out_t *out, uint32_t pdesc, 
               otree_t *RESTRICT tree, uint32_t node) {
  int oct;

  uint32_t *octs = lst_get(tree,node).octs;
  uint32_t leaf_mask = 0; //non-leaf mask
  uint32_t valid_mask = 0;
  for (oct = 0; oct < 8; oct++) {
    uint32_t voxel = octs[oct];
    if (OT_COLOR(voxel)) {
      valid_mask |= OB(oct);
      if (voxel&OT_TERM) {
        leaf_mask |= OB(oct);
      }
    }
  }
  uint32_t branch_mask = (leaf_mask^0xff)&valid_mask;
  int nkids = popc8(branch_mask);
  uint32_t pkids = out->ptr;
  out->ptr += 2*nkids;
  
  uint32_t okids = (pkids-pdesc)>>1; //offset to kids
  //if (!nkids) okids = 0;

  // write descriptor
  out->base[pdesc+0] = (okids<<17) | (valid_mask<<8) | branch_mask;
  out->base[pdesc+1] = 0; //contours
  /*fprintf(stderr, "%d:build: desc=%x, nkids=%x\n"
                , node, out->base[pdesc+0], nkids);*/

  int kid = 0;
  for (oct = 0; oct < 8; oct++) {
    if (!(valid_mask&OB(oct))) continue;
    if (leaf_mask&OB(oct)) {
      //fprintf(stderr, "%d:childF: %d\n", node,oct);
    } else {
      //fprintf(stderr, "%d:childS: %d\n", node,oct);
      ot2nvsvo_r(out, pkids+kid*2, tree, octs[oct]);
      kid++;
    }
  }
}

#undef OB

//if it is commented, then contour field is used to point to attribs
//#define SVO_USE_ALUT 1

//descriptor size
#define SVO_DSZ 2

#define SVO_ERASE 0xFFFFFFFF

typedef struct {
  uint32_t color;
  uint32_t normal;
} svo_attr_t;

lst_def(svo_attr_lst_t,svo_attr_t);

typedef struct {
  uint32_t_lst_t topo;  //topology
#ifdef SVO_USE_ALUT
  uint32_t_lst_t alut;  //maps topology to attributes
#endif
  svo_attr_lst_t attr;  //attributes
  uint32_t c;           //cube edge size (1<<scale)
} svo_t;


typedef struct {
  ivec3 o;        //octant x,y,z origin
  uint32_t c;     //octant edge size (center of the parent voxel)
  uint32_t node;  //parent node offset of this leaf
  uint32_t oct;   //octant index of this leaf
} leaf_t;


static INLINE void svo_init(svo_t *svo, int scale) {
  svo->c = 1<<scale;
  lst_init(&svo->topo,128);
#ifdef SVO_USE_ALUT
  lst_init(&svo->alut,128);
#endif
  lst_init(&svo->attr,128);
  lst_push(&svo->topo,1<<17); //pointer to the first child
  lst_push(&svo->topo,0);
  lst_push(&svo->topo,0); //FIXME: omit it when c==1
  lst_push(&svo->topo,0);
#ifdef SVO_USE_ALUT
  lst_push(&svo->alut,0);
#endif
}

static INLINE void svo_free(svo_t *svo) {
  lst_free(&svo->topo);
#ifdef SVO_USE_ALUT
  lst_free(&svo->alut);
#endif
  lst_free(&svo->attr);
}

static INLINE void svo_clear(svo_t *svo, uint32_t color) {
  int c = svo->c;
  svo_free(svo);
  svo_init(svo, 0);
  svo->c = c;
  svo->topo.elts[0] = (1<<17) | 0xFF00;
  svo->topo.elts[1] = 0;

  svo_attr_t value;
  value.color = color;
  value.normal = 0;

#ifdef SVO_USE_ALUT
  uint32_t *alut = svo->alut.elts;
  alut[0] = 0;
#endif

  for (int i = 0; i < 8; i++) {
    lst_push(&svo->attr,value);
  }
}

lst_def(leaf_lst_t,leaf_t);

static ivec3 ioct2gt[] = {
  {0,0,0},{1,0,0},
  {0,1,0},{1,1,0},
  {0,0,1},{1,0,1},
  {0,1,1},{1,1,1}
};

static INLINE svo_attr_t *
svo_attr(svo_t *svo, uint32_t *node, uint32_t bit, uint32_t valid_mask) {
  int kindex = popc8(valid_mask&(bit-1));
#ifdef SVO_USE_ALUT
  uint32_t *alut = svo->alut.elts;
  uint32_t oattr = alut[node-svo->topo.elts];
#else
  uint32_t oattr = node[1];
#endif
  return &svo->attr.elts[oattr+kindex];
}

#define SVO_NODE_MASKS(node,valid_mask,branch_mask)  \
    uint32_t valid_mask = (*node>>8)&0xFF;           \
    uint32_t branch_mask = *node&0xFF;

#define SVO_NODE_KIDS(node,pkids)                              \
    uint32_t *pkids;                                           \
    {uint32_t okids = *node>>17; /*offset to the child list*/  \
     if (*node&0x10000) okids = node[okids*2]; /*far pointer*/ \
     pkids = node + okids*2;}


static INLINE svo_attr_t *svo_get(svo_t *svo, ivec3 p) {
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
    if (!(branch_mask&bit)) return svo_attr(svo, node, bit, valid_mask);
    SVO_NODE_KIDS(node,pkids);
    int kindex = popc8(branch_mask&(bit-1));
    node = pkids + kindex*2;
    c >>= 1;
  }
}


static INLINE void svo_set(svo_t *svo, ivec3 p, uint32_t color) {
  uint32_t *node = svo->topo.elts;
  ivec3 axes = {1,2,4};
  svo_attr_t value;
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
    uint32_t node_ofs = (node-svo->topo.elts)>>1;
    if (!c) {
      if (color == SVO_ERASE) {
        if (valid_mask&bit) {
#ifdef SVO_USE_ALUT
          uint32_t k = lst_get(&svo->alut, node_ofs);
#else
          uint32_t k = node[1];
#endif
          uint32_t i = k+popc8(valid_mask&(bit-1));
          uint32_t n = k+popc8(valid_mask);
          for ( ; i < n-1; i++) { //shift attrs, omitting the erased leaf
            lst_get(&svo->attr,i) = lst_get(&svo->attr,i+1);
          }
          *node ^= bit<<8; //erase it
        }
        return;
      }
      if (valid_mask&bit) {
#ifdef SVO_USE_ALUT
        uint32_t oattr = lst_get(&svo->alut, node_ofs); //node kids attribs
#else
        uint32_t oattr = node[1];
#endif
        oattr += popc8(valid_mask&(bit-1)); //offset to this child attribs
        lst_get(&svo->attr,oattr) = value;
      } else { //expand it to include this octant
#ifdef SVO_USE_ALUT
        uint32_t k = lst_get(&svo->alut, node_ofs);
        lst_get(&svo->alut, node_ofs) = svo->attr.used;
#else
        uint32_t k = node[1];
        node[1] = svo->attr.used;
#endif
        valid_mask |= bit;
        uint32_t bit_i = popc8(valid_mask&(bit-1));
        uint32_t n = popc8(valid_mask);
        for (uint32_t i = 0; i < n; i++) {
          if (i != bit_i) {
            lst_push(&svo->attr, lst_get(&svo->attr,k++));
          } else {
            lst_push(&svo->attr, value);
          }
        }
        *node |= bit<<8; //update valid_mask
      }
      return;
    }
    if (!(valid_mask&bit) || !(branch_mask&bit)) {
      uint32_t node_ofs = node - svo->topo.elts;
      uint32_t naddr = node_ofs + (*node>>17)*2;
      uint32_t ndesc = (valid_mask&bit) ? 0xff00 : 0x0000;
      uint32_t next_desc_ofs;
      if (c != 1 || branch_mask) { //have to copy branches?
        SVO_NODE_KIDS(node,pkids);
        uint32_t prev = pkids - svo->topo.elts;
        uint32_t next = svo->topo.used - node_ofs;
        for (int i = 0; i < 8; i++) {
          if (branch_mask&(1<<i)) {
            //FIXME: desc kids offsets have to be updated too!
            lst_push(&svo->topo,lst_get(&svo->topo,prev));
            prev++;
            lst_push(&svo->topo,lst_get(&svo->topo,prev));
            prev++;
          } else if (i == oct) {
            next_desc_ofs = svo->topo.used;
            lst_push(&svo->topo,0); //desc
            lst_push(&svo->topo,0); //contours
          }
        }
        if (c != 1) { //node needs at least one branch descriptor
          uint32_t ndesc_okids = svo->topo.used - next_desc_ofs;
          lst_push(&svo->topo,0);
          lst_push(&svo->topo,0);
          ndesc |= (ndesc_okids>>1)<<17;
        } else {
          ndesc |= 1<<17;
        }
        lst_get(&svo->topo,next_desc_ofs) = ndesc;
        node = &lst_get(&svo->topo,node_ofs);
        *node |= 0x10000 | (bit<<8) | bit;
        lst_get(&svo->topo,naddr) = next>>1;
      } else { //when c==1, existing empty branch can be reused
        *node |= (bit<<8) | bit;
        *node &= ~(uint32_t)0x10000;
        ndesc |= 1<<17;
        lst_get(&svo->topo,naddr) = ndesc;
        lst_get(&svo->topo,naddr+1) = 0;
        next_desc_ofs = naddr;
      }
#ifdef SVO_USE_ALUT
      next_desc_ofs >>= 1;
      lst_eset(&svo->alut,next_desc_ofs,0); //ensure attrib list gets expanded
      uint32_t *alut = svo->alut.elts;
#endif
      if (valid_mask&bit) { //existing leaf to split?
#ifdef SVO_USE_ALUT
        uint32_t oattr = alut[node_ofs>>1];
        //alut[node_ofs>>1] = <FIXME: CALC AVG OF KID ATTRIBS>;
#else
        uint32_t oattr = node[1];
#endif
        oattr += popc8(valid_mask&(bit-1));
        svo_attr_t prev_attr = lst_get(&svo->attr,oattr);
#ifdef SVO_USE_ALUT
        alut[next_desc_ofs] = svo->attr.used;
#else
        node[next_desc_ofs+1] = svo->attr.used;
#endif
        for (int i = 0; i < 8; i++) {
          lst_push(&svo->attr,prev_attr);
        }
      } else { //splitting empty leaf: no attrib
#ifdef SVO_USE_ALUT
        alut[next_desc_ofs] = 0;
#else
        node[1] = 0;
#endif
      }
    }

    SVO_NODE_KIDS(node,pkids);
    int kindex = popc8(branch_mask&(bit-1));
    node = pkids + kindex*2;
  }
}


static void svo_list_terms2(int valid
                    ,leaf_lst_t *list, uint32_t *base, uint32_t node_ofs
                    ,int c, ivec3 pos)
{
  uint32_t *node = base+node_ofs;
  SVO_NODE_MASKS(node,valid_mask,branch_mask);
  SVO_NODE_KIDS(node,pkids);
  uint32_t kid = pkids-base;
  int oct;
  for (oct = 0; oct < 8; oct++) {
    vec3 oct_pos = pos + ioct2gt[oct]*c;
    if (branch_mask&(1<<oct)) {
      svo_list_terms2(valid, list, base, kid, c>>1, oct_pos);
      kid += 2;
    } else if (valid == 2                    ||
               ( valid &&  (valid_mask&oct)) ||
               (!valid && !(valid_mask&oct))) {
      lst_add(leaf_index,list);
      leaf_t *leaf = &lst_get(list,leaf_index);
      leaf->o = oct_pos;
      leaf->c = c;
      leaf->node = node_ofs;
      leaf->oct = oct;
    }
  }
}

#define svo_leaf_list(svo, list) \
  leaf_lst_t list;               \
  lst_init(&list,128);           \
  svo_list_terms2(1, &list, svo->topo.elts, 0, svo->c, (ivec3){0,0,0});

#define svo_empty_list(svo, list) \
  leaf_lst_t list;               \
  lst_init(&list,128);           \
  svo_list_terms2(0, &list, svo->topo.elts, 0, svo->c, (ivec3){0,0,0});

#define svo_term_list(svo, list) \
  leaf_lst_t list;               \
  lst_init(&list,128);           \
  svo_list_terms2(2, &list, svo->topo.elts, 0, svo->c, (ivec3){0,0,0});

static svo_attr_t *svo_term_attr(svo_t *svo, leaf_t *leaf) {
  uint32_t *node = svo->topo.elts+leaf->node;
  SVO_NODE_MASKS(node,valid_mask,branch_mask);
  return svo_attr(svo, node, 1<<leaf->oct, valid_mask);
}

static void svo_print_term_list(svo_t *svo, leaf_lst_t *list) {
  for (int i = 0; i < list->used; i++) {
    leaf_t *t = &list->elts[i];
    uint32_t *node = svo->topo.elts + t->node;
    SVO_NODE_MASKS(node,valid_mask,branch_mask);
    int exist = valid_mask&(1<<t->oct) ? 1 : 0;
    uint32_t color = 0xFFFFFFFF;
    if (exist) {
      color = svo_term_attr(svo, t)->color;
    }
    fprintf(stderr, "(%d,%d,%d):%d:%06x+%d:%s:color=#%08x\n"
           ,t->o.x, t->o.y, t->o.z, t->c
           ,t->node>>1, t->oct
           ,exist ? "exist" : "empty"
           ,color);
  }
}

static void svo_print_terms(svo_t *svo) {
  svo_term_list(svo, list);
  svo_print_term_list(svo, &list);
}



#define OB(oct) (1<<(oct))

#if 0
static
void svo2nvsvo_r(svo_t *svo, int c, uint32_t pdesc, 
               otree_t *RESTRICT tree, uint32_t node) {
  int oct;

  uint32_t *octs = lst_get(tree,node).octs;
  uint32_t leaf_mask = 0; //non-leaf mask
  uint32_t valid_mask = 0;
  for (oct = 0; oct < 8; oct++) {
    uint32_t voxel = octs[oct];
    if (OT_COLOR(voxel)) {
      valid_mask |= OB(oct);
      if (voxel&OT_TERM) {
        leaf_mask |= OB(oct);
      }
    }
  }
  uint32_t branch_mask = (leaf_mask^0xff)&valid_mask;
  int nkids = popc8(branch_mask);
  uint32_t pkids = svo->topo.used;
  
  uint32_t okids = (pkids-pdesc)>>1; //offset to kids
  //if (!nkids) okids = 0;

  // write descriptor
  uint32_t desc = 0x10000 | (okids<<17) | (valid_mask<<8) | branch_mask;
  svo->topo.elts[pdesc+0] = desc;
  svo->topo.elts[pdesc+1] = 0; //contours
  /*fprintf(stderr, "%d:build: desc=%x, nkids=%x\n"
                , node, out->base[pdesc+0], nkids);*/

  for (oct = 0; oct < 8; oct++) {
    if (!(valid_mask&OB(oct))) continue;
    lst_push(&svo->topo, 0);
    lst_push(&svo->topo, 0);
  }

  int kid = 0;
  for (oct = 0; oct < 8; oct++) {
    if (!(valid_mask&OB(oct))) continue;
    if (leaf_mask&OB(oct)) {
      //fprintf(stderr, "%d:childF: %d\n", node,oct);
    } else {
      //fprintf(stderr, "%d:childS: %d\n", node,oct);
      svo2nvsvo_r(out, c>>1, pkids+kid*2, tree, octs[oct]);
      kid++;
    }
  }
}

#undef OB

static void svo2nvsvo_r(svo_t *svo, otree_t *RESTRICT tree, int c) {
  svo_free(svo);
  svo_init(svo, 1);
  svo->c = c;
  svo2nvsvo_r(&svo, svo.c, 0, &slb->tree, 0);
}
#endif

#endif //SVO_H