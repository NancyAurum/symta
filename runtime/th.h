// Text-keyed Symta hash map (AM_TEXT backing).
//
// Drops the last stb_ds use in the adaptive map. After this
// header, all three "real" hash modes (AM_GENERIC via dh,
// AM_INT via ih, AM_TEXT via th) share the same nh_t template,
// Robin Hood probing, 75% load factor, and (for dh/th) the
// AM-15 hash cache. AM_BITMAP* stays on the dedicated nb_t
// (sparse-bitmap, no probing).
//
// Key encoding:
//   keys are Symta `dyn` values constrained to text (FIXTEXT
//   or BIGTEXT). Real text dyns always have a non-zero tag
//   (T_FIXTEXT=2, T_TEXT=13). FXN(0) = 0 has tag T_INT = 0,
//   so it can never collide with a text dyn -- we use it as
//   the empty-slot sentinel.
//
// Hash choice:
//   - FIXTEXT: Murmur3 finaliser on the full 64-bit dyn. The
//     dyn IS the encoded text (chars packed into bits 16-55),
//     so feeding it into a strong mixer gives a uniform hash.
//     A naive low^high fold leaves the variable chars (the
//     trailing digits in "key_<N>" patterns) in the upper half
//     where they don't reach the cap mask -- catastrophic
//     clustering, AM-6b's first investigation.
//   - BIGTEXT: FNV-1a over the bytes. Adler-32 (which dh.h
//     uses) clusters on common-prefix inputs because its low
//     16 bits are just a running byte sum -- "key_100" through
//     "key_4999" all land in a 100-slot range. FNV-1a's
//     per-byte multiply-then-xor spreads varying-byte entropy
//     across the whole hash word.
//
// Equality:
//   texts_equal handles mixed FIXTEXT vs BIGTEXT correctly
//   (alloc_text picks FIXTEXT for short strings, so a BIGTEXT
//   key has content too long to fit a FIXTEXT and cross-type
//   equality is always false).
//
// GC note:
//   th_t stores text dyns directly, not arena-copied char*.
//   gc_types.h::tbl_gc_internals AM_TEXT branch walks both
//   keys and values; otherwise a BIGTEXT key whose only live
//   reference is the slot would get reclaimed.
//
// Iteration order:
//   th_t iterates in Robin Hood / hash order, which is *not*
//   the order keys were inserted. Symta tables are explicitly
//   unordered hash maps (see examples/18-tables.s); user code
//   that needs deterministic iteration sorts T.l.s or T.ks.s
//   first.

#ifndef TH_H
#define TH_H

// Requires dh.h to have been included so we can reuse dhFmix64_
// (the Murmur3 finaliser) without duplicating it.

#define NH_PFX th
#define NH_KEY dyn
#define NH_VAL dyn
#define NH_KEY_NIL ((dyn)0)
#define NH_VAL_NIL NIL
#define NH_PREP

#define NH_ROBIN_HOOD
#define NH_NEEDS_GROW(nh) ((nh)->used * 4 > ((nh)->cap + 1) * 3)
#define NH_CACHE_HASH

#define NH_INIT_VAL(o) o = 0

INLINE uint32_t thFnv1a_(const uint8_t *data, int len) {
  uint32_t h = 0x811C9DC5u;          // FNV offset basis
  for (int i = 0; i < len; ++i) {
    h ^= data[i];
    h *= 0x01000193u;                // FNV prime
  }
  return h;
}

INLINE uint32_t thHash_(dyn key) {
  uint32_t t = O_TAG(key);
  if (t == T_FIXTEXT) {
    return (uint32_t)dhFmix64_((uint64_t)key);
  }
  return thFnv1a_((const uint8_t*)BIGTEXT_DATA(key), BIGTEXT_SIZE(key));
}

INLINE int thEqual_(dyn a, dyn b) {
  return texts_equal(a, b);
}

#define NH_HASH(o) thHash_(o)
#define NH_EQUAL(a,b) thEqual_(a,b)

#include "nh.h"

#endif
