//fully dynamically typed hash map
#ifndef DH_H
#define DH_H

#define NH_PFX dh
#define NH_KEY dyn
#define NH_VAL dyn
#define NH_KEY_NIL NIL
#define NH_VAL_NIL NIL
#define NH_PREP

//#define NH_ZERO_VALS
#define NH_INIT_VAL(o) o = 0

// AM-3 (TODO.md): inline int/text equality and hashing so the
// dominant 90%+ of "generic" tables (which in practice are still
// int- or text-keyed with one stray odd-typed key forcing GENERIC
// mode) don't pay a full Symta method dispatch on every probe step
// of every lookup, insert, delete, and grow.
//
// The inlined hash values MUST agree bit-for-bit with what
// int.hash / text.hash return through MCALL, otherwise a table
// populated through one path won't be findable through the other.
// Both paths boil down to the same C helpers below.

INLINE uint64_t dhFmix64_(uint64_t key) {
  // Identical to fmix64 in bltin.c (kept in sync; both are the
  // Murmur3 finaliser). Inlined here because that one is `static`
  // to its TU and the header can't reach it.
  key ^= key >> 33;
  key *= 0xff51afd7ed558ccdULL;
  key ^= key >> 33;
  key *= 0xc4ceb9fe1a85ec53ULL;
  key ^= key >> 33;
  return key;
}

INLINE uint32_t dhAdler_(const uint8_t *data, int len) {
  // Identical to `hash` in bltin.c. Adler-32 variant used by
  // text.hash; (low 32 bits is what dhHash_ ultimately returns).
  #define DH_MOD_ADLER 65521
  uint32_t a = 1, b = 0;
  for (int i = 0; i < len; ++i) {
    a = (a + data[i]) % DH_MOD_ADLER;
    b = (b + a)       % DH_MOD_ADLER;
  }
  return (b << 16) | a;
  #undef DH_MOD_ADLER
}

INLINE dyn dhEqual_(dyn a, dyn b) {
  // Fast-path: same-tag int and text comparisons skip MCALL entirely.
  // Mixed-tag (int vs text, etc.) cannot match and returns FXN(0)
  // immediately without dispatch.
  uint32_t ta = O_TAG(a);
  uint32_t tb = O_TAG(b);
  if (ta == T_INT && tb == T_INT)         return FXN(a == b);
  if (ta == T_FIXTEXT && tb == T_FIXTEXT) return FXN(a == b);
  if (ta == T_TEXT && tb == T_TEXT) {
    int al = BIGTEXT_SIZE(a);
    int bl = BIGTEXT_SIZE(b);
    return FXN(al == bl && !memcmp(BIGTEXT_DATA(a),BIGTEXT_DATA(b),al));
  }
  // FIXTEXT and TEXT have different tags but can compare equal:
  // a 7-char text "hello" can live in either representation. Defer
  // to the existing helper that handles the mixed case correctly.
  if ((ta == T_FIXTEXT || ta == T_TEXT) &&
      (tb == T_FIXTEXT || tb == T_TEXT)) {
    return FXN(texts_equal(a, b));
  }
  // Genuine user type / cross-type / anything we can't inline ->
  // fall back to the original method-dispatch path.
  GC_DISABLE();
  dyn r;
  ARGLIST2(a,b);
  MCALL(r,a,api.m_equal);
  GC_ENABLE();
  return r;
}

INLINE uint32_t dhHash_(dyn key) {
  uint32_t t = O_TAG(key);
  if (t == T_INT) {
    // int.hash returns FXN(fmix64(key) & GID_MASK_NO_SIGN); the
    // dhHash_ caller truncates back to uint32_t. The mask only
    // touches bits above bit 31 so we can skip it here.
    return (uint32_t)dhFmix64_((uint64_t)key);
  }
  if (t == T_FIXTEXT) {
    // text.hash on fixtext folds the 8-byte representation in half.
    uint64_t v = (uint64_t)key;
    return (uint32_t)((v & 0xFFFFFFFFu) ^ (v >> 32));
  }
  if (t == T_TEXT) {
    return dhAdler_((const uint8_t*)BIGTEXT_DATA(key), BIGTEXT_SIZE(key));
  }
  // Anything else (user type, list, closure, …) -> MCALL.
  GC_DISABLE();
  dyn r;
  ARGLIST1(key);
  MCALL(r,key,api.m_hash);
  GC_ENABLE();
  return (uint32_t)UNFXN(r);
}

#define NH_EQUAL(a,b) dhEqual_(a,b)
#define NH_HASH(o) dhHash_(o)

//#define NH_NEEDS_GROW(nh) ((nh)->used >= (((nh)->cap+1)>>2)*3)

#include "nh.h"


#endif
