// Integer-keyed Symta hash map (AM_INT backing).
//
// Drops the stb_ds dependency in favour of the nh_t template
// that AM_GENERIC already uses, with the Robin Hood + 75% load
// configuration from dh.h. AM-6 called this out as the
// next consolidation step after AM-5 -- two probing strategies
// and two memory layouts make perf work twice as expensive.
//
// Key encoding:
//   keys are Symta `dyn` values. Real T_INT keys are FXN(n) =
//   n<<GID_SHFT, so their low GID_SHFT bits are always zero.
//   We pick `NIL` (= HEAPREF(0) = 1, low bit = T_HEAP) as the
//   empty-slot sentinel: it can never collide with FXN(n) since
//   FXN(n)'s low bits are zero. (Yes, FXN(0) is itself the
//   value 0, which is also the most natural "empty" sentinel
//   for stb_ds-style tables. We can't reuse that here because
//   nh_t reads keys[i] to check emptiness.)
//
// Hash:
//   the Murmur3 64-bit finaliser, identical bits to what
//   int.hash returns through the MCALL path and to dh.h's
//   T_INT fast path. Inlined directly so the per-probe cost
//   is a single multiply-shift sequence.

#ifndef IH_H
#define IH_H

#define NH_PFX ih
#define NH_KEY dyn
#define NH_VAL dyn
#define NH_KEY_NIL NIL
#define NH_VAL_NIL NIL
#define NH_PREP

// Robin Hood + 75% load -- same configuration as dh.h.
#define NH_ROBIN_HOOD
#define NH_NEEDS_GROW(nh) ((nh)->used * 4 > ((nh)->cap + 1) * 3)

#define NH_INIT_VAL(o) o = 0

INLINE uint64_t ihFmix64_(uint64_t key) {
  // Same as dhFmix64_ / int.hash's fmix64. Kept locally so the
  // header can be used standalone; they're both copies of the
  // same Murmur3 finaliser and must stay in sync.
  key ^= key >> 33;
  key *= 0xff51afd7ed558ccdULL;
  key ^= key >> 33;
  key *= 0xc4ceb9fe1a85ec53ULL;
  key ^= key >> 33;
  return key;
}

#define NH_HASH(o) ((uint32_t)ihFmix64_((uint64_t)(o)))
#define NH_EQUAL(a,b) ((a)==(b))

#include "nh.h"


#endif
