//Nancy's Hashmap
#include <malloc.h>
#include <stdint.h>

/*
Points to consider:
* Values can be allocated together and just adding cap to the nh_t

* It is possible to implement lazy deletion,
  where actual deletion happens on growth or when some threshold reached

* separating index eleminates the need for NH_NIL_KEY
  It c an also save a bit of space, when the values are large,
  since key/value space can grow only when
  it is required, while index will be a pointer.
  Yet it can incur additional cache misses.
  Consider scenario where table of size S stores S/2 key-value pairs
  I.e. we double the table size on each grow event.
  If both keys and values are 64bit, we waste about (S/2)*8*2 bytes.
  With 32bit index we will waste just (S/2)*8.
  On growth events we also wont need to copy the keys or values,
  Just update the index. In fact we wont even toch the values at all.
  Sequentially added keys-values are morely likely to be accessed at
  the same time.
  Although we do need grow the arrays as keys and values get added.

* A separate 32bit index can also speed up deletion, as well as
  enumerating keys/values.
  
* Should we keep keys and values in separate arrays, as an array of pairs?

* If we keep nh_t elements separate from index,
  deleted element could be replaced by the element from the end,
  given that it's index gets updated.

*/

//ng.h defines PRFX, but we also want this header to be usable standalone
#ifndef PRFX
#define PRFX_(prfx,mid,token) prfx##mid##token
#define PRFX(prfx,mid,token) PRFX_(prfx,mid,token)
#endif

#ifndef NH_KEY
#define NH_KEY uint32_t
#endif

#ifndef NH_VAL
#define NH_VAL uint32_t
#endif

#ifndef NH_KEY_NIL
#define NH_KEY_NIL 0
#endif

#ifndef NH_PFX
#define NH_PFX nh
#endif

//Large class of cases doesn't require initing values
//#define NH_INIT_VAL(x) x = 0

#define nh_t PRFX(NH_PFX,,_t)


typedef struct {
  uint32_t cap; //capacity
  uint32_t used;
  NH_VAL *vals;
  NH_KEY keys[1];
} nh_t;

#define NH_INIT_SZ (1<<2)

#ifndef NH_NEEDS_GROW
//4/8 full
//#define NH_NEEDS_GROW(nh) ((nh)->used >= (((nh)->cap+1)>>1))

//5/8 full
//#define NH_NEEDS_GROW(nh) ((nh)->used >= (((nh)->cap+1)>>3)*5)

//6/8 full
//#define NH_NEEDS_GROW(nh) ((nh)->used >= (((nh)->cap+1)>>2)*3)

#define NH_NEEDS_GROW(nh) ((nh)->cap < (nh)->used*2)
#endif

#ifndef NH_HASH
/*INLINE uint64_t PRFX(NH_PFX,,Hash_)(uint64_t key) {
  key ^= key >> 33;
  key *= 0xff51afd7ed558ccd;
  key ^= key >> 33;
  key *= 0xc4ceb9fe1a85ec53;
  key ^= key >> 33;
  return key;
}*/

INLINE uint64_t PRFX(NH_PFX,,Hash_)(uint64_t key) {
  return (key ^ (key >> 33)) * 0xff51afd7ed558ccd;
}

#define NH_HASH(o) PRFX(NH_PFX,,Hash_)(o)
#endif

#ifndef NH_EQUAL
#define NH_EQUAL(a,b) ((a)==(b))
#endif

#ifndef NH_ALLOC
#define NH_ALLOC(size) malloc(size);
#endif

#ifndef NH_FREE
#define NH_FREE(size) free(size);
#endif

//keys should be initialized to 0
INLINE nh_t *PRFX(NH_PFX,,Alloc_)(int cap) {
  nh_t *nh = (nh_t*)NH_ALLOC(sizeof(nh_t)+sizeof(NH_KEY)*(cap-1));
  nh->vals = NH_ALLOC(sizeof(NH_VAL)*cap);
#ifdef NH_ZERO_VALS
  memset(nh->vals, 0, sizeof(NH_VAL)*cap);
#undef NH_ZERO_VALS
#endif

  nh->cap = cap-1;
  nh->used = 0;
  NH_KEY *ks = nh->keys;
  for (int i = 0; i < cap; i+=4) {
    ks[i  ] = NH_KEY_NIL;
    ks[i+1] = NH_KEY_NIL;
    ks[i+2] = NH_KEY_NIL;
    ks[i+3] = NH_KEY_NIL;
  }
  return nh;
}

INLINE nh_t *PRFX(NH_PFX,,Alloc)() {
#ifdef NH_PREP
  nh_t *nh = PRFX(NH_PFX,,Alloc_)(NH_INIT_SZ);
#else
  nh_t *nh = 0;
#endif
  return nh;

}

INLINE void PRFX(NH_PFX,,Free)(nh_t *nh) {
#ifndef NH_PREP
  if (nh)
#endif
  {
    NH_FREE(nh->vals);
    NH_FREE(nh);
  }
}

// Add_, Lookup, Get, Del exist in two flavours below: a plain
// linear-probing variant (the original) and a Robin Hood variant
// guarded by NH_ROBIN_HOOD. Define NH_ROBIN_HOOD before including
// this header to opt the instantiation in.
//
// Robin Hood probing (Pedro Celis, 1986; pioneered for hash tables
// by Emmanuel Goossaert / Malte Skarupke) bounds the variance of
// probe lengths by ensuring entries are sorted by distance-from-
// initial-bucket (DIB) along the probe chain. When inserting, if
// we encounter a slot whose resident DIB is LESS than ours, we
// take their slot and the resident continues down the chain.
// Effect: at 75-80% load factor, average and worst-case probe
// lengths stay close (typically 1-3 probes) whereas plain linear
// probing degrades quadratically -- avg ~9 probes for a miss at
// 75%. Lookups also gain an early-exit: if we see a resident
// whose DIB < ours, our key cannot be in this chain (otherwise
// Robin Hood would have placed it before this slot).
//
// The DIB isn't stored; we recompute it on the fly via
//   dib(slot_i) = (slot_i - (NH_HASH(keys[slot_i]) & cap)) & cap
// which is exact because the table size is a power of two. This
// avoids the per-slot byte of overhead a "DIB array" variant
// would cost.

#ifdef NH_ROBIN_HOOD

INLINE NH_VAL *PRFX(NH_PFX,,Add_)(nh_t *nh, NH_KEY key, int *old) {
  uint32_t cap = nh->cap;
  NH_KEY *ks = nh->keys;
  NH_VAL *vs = nh->vals;
  uint32_t home_my = NH_HASH(key) & cap;
  uint32_t i = home_my;
  uint32_t dib_my = 0;

  for ( ; ks[i] != NH_KEY_NIL; i = (i+1)&cap, dib_my++) {
    if (NH_EQUAL(ks[i], key)) {
      if (old) *old = 1;
      return vs+i;
    }
    uint32_t home_them = NH_HASH(ks[i]) & cap;
    uint32_t dib_them = (i - home_them) & cap;
    if (dib_them < dib_my) {
      // Robin Hood swap: install our key at this slot, take the
      // resident along on the chain. The new entry's val isn't
      // known yet (the caller writes through the returned pointer),
      // so we remember the slot and re-insert the displaced one.
      NH_KEY  k_disp = ks[i];
      NH_VAL  v_disp = vs[i];
      ks[i] = key;
      NH_VAL *ret = vs + i;
#ifdef NH_INIT_VAL
      NH_INIT_VAL(*ret);
#endif
      nh->used++;
      if (old) *old = 0;

      // Inner loop: keep displacing until we hit an empty slot.
      uint32_t j  = (i+1) & cap;
      NH_KEY   k  = k_disp;
      NH_VAL   v  = v_disp;
      uint32_t home_j = home_them;
      uint32_t dib_j  = (j - home_j) & cap;
      for (;; j = (j+1)&cap, dib_j++) {
        if (ks[j] == NH_KEY_NIL) {
          ks[j] = k;
          vs[j] = v;
          return ret;
        }
        uint32_t home_jx = NH_HASH(ks[j]) & cap;
        uint32_t dib_jx  = (j - home_jx) & cap;
        if (dib_jx < dib_j) {
          NH_KEY kt = ks[j]; NH_VAL vt = vs[j];
          ks[j] = k; vs[j] = v;
          k = kt; v = vt;
          home_j = home_jx;
          dib_j  = dib_jx;
        }
      }
      // unreachable: the inner loop only exits via the empty slot.
    }
  }

  // Empty slot found before any Robin Hood swap.
  ks[i] = key;
  nh->used++;
  if (old) *old = 0;
#ifdef NH_INIT_VAL
  NH_INIT_VAL(vs[i]);
#endif
  return vs+i;
}

INLINE NH_VAL *PRFX(NH_PFX,,Lookup)(nh_t *nh, NH_KEY key) {
#ifndef NH_PREP
  if (!nh) return 0;
#endif
  uint32_t cap = nh->cap;
  NH_KEY *ks = nh->keys;
  uint32_t home_my = NH_HASH(key) & cap;
  uint32_t i = home_my;
  uint32_t dib_my = 0;
  for ( ; ks[i] != NH_KEY_NIL; i = (i+1)&cap, dib_my++) {
    if (NH_EQUAL(ks[i], key)) return nh->vals+i;
    uint32_t home_them = NH_HASH(ks[i]) & cap;
    uint32_t dib_them = (i - home_them) & cap;
    if (dib_them < dib_my) return 0; // RH invariant -> not in table
  }
  return 0;
}

#ifdef NH_VAL_NIL
INLINE NH_VAL PRFX(NH_PFX,,Get)(nh_t *nh, NH_KEY key) {
#ifndef NH_PREP
  if (!nh) return NH_VAL_NIL;
#endif
  uint32_t cap = nh->cap;
  NH_KEY *ks = nh->keys;
  uint32_t home_my = NH_HASH(key) & cap;
  uint32_t i = home_my;
  uint32_t dib_my = 0;
  for ( ; ks[i] != NH_KEY_NIL; i = (i+1)&cap, dib_my++) {
    if (NH_EQUAL(ks[i], key)) return nh->vals[i];
    uint32_t home_them = NH_HASH(ks[i]) & cap;
    uint32_t dib_them = (i - home_them) & cap;
    if (dib_them < dib_my) return NH_VAL_NIL;
  }
  return NH_VAL_NIL;
}
#undef NH_VAL_NIL
#endif

#else // NH_ROBIN_HOOD -- the original linear-probing variant.

INLINE NH_VAL *PRFX(NH_PFX,,Add_)(nh_t *nh, NH_KEY key, int *old) {
  uint32_t cap = nh->cap;
  uint32_t i = NH_HASH(key)&cap;
  NH_KEY *ks = nh->keys;

  for ( ; ks[i] != NH_KEY_NIL; i = (i+1)&cap) {
    if (NH_EQUAL(ks[i], key)) {
      if (old) *old = 1;
      return nh->vals+i;
    }
  }

  ks[i] = key;
  nh->used++;
  if (old) *old = 0;
#ifdef NH_INIT_VAL
  NH_INIT_VAL(nh->vals[i]);
#endif
  return nh->vals+i;
}


INLINE NH_VAL *PRFX(NH_PFX,,Lookup)(nh_t *nh, NH_KEY key) {
#ifndef NH_PREP
  if (!nh) return 0;
#endif
  uint32_t cap = nh->cap;
  uint32_t i = NH_HASH(key)&cap;
  NH_KEY *ks = nh->keys;
  for ( ; ks[i] != NH_KEY_NIL; i = (i+1)&cap) {
    if (NH_EQUAL(ks[i], key)) return nh->vals+i;
  }
  return 0;
}


#ifdef NH_VAL_NIL
INLINE NH_VAL PRFX(NH_PFX,,Get)(nh_t *nh, NH_KEY key) {
#ifndef NH_PREP
  if (!nh) return NH_VAL_NIL;
#endif
  uint32_t cap = nh->cap;
  uint32_t i = NH_HASH(key)&cap;
  NH_KEY *ks = nh->keys;
  for ( ; ks[i] != NH_KEY_NIL; i = (i+1)&cap) {
    if (NH_EQUAL(ks[i], key)) return nh->vals[i];
  }
  return NH_VAL_NIL;
}
#undef NH_VAL_NIL
#endif

#endif // NH_ROBIN_HOOD


INLINE int PRFX(NH_PFX,,Cap)(nh_t *nh) {
#ifdef NH_PREP
  return nh->cap+1;
#else
  return nh ? nh->cap+1 : 0;
#endif
}

INLINE int PRFX(NH_PFX,,N)(nh_t *nh) {
#ifdef NH_PREP
  return nh->used;
#else
  return nh ? nh->used : 0;
#endif
}

INLINE void PRFX(NH_PFX,,Grow_)(nh_t **pnh) {
  nh_t *onh = *pnh;
  uint32_t cap = onh->cap+1;
  uint32_t new_cap = cap*2;
  nh_t *nh = PRFX(NH_PFX,,Alloc_)(new_cap);
  *pnh = nh;
  NH_KEY *ks = onh->keys; 
  NH_VAL *vs = onh->vals;
  for (int i = 0; i < cap; i++) {
    if (ks[i] == NH_KEY_NIL) continue;
    *PRFX(NH_PFX,,Add_)(nh,ks[i],0) = vs[i];
  }
  PRFX(NH_PFX,,Free)(onh);
}



INLINE NH_VAL *PRFX(NH_PFX,,Add)(nh_t **pnh, NH_KEY key, int *old) {
#ifndef NH_PREP
  if (!*pnh) *pnh = PRFX(NH_PFX,,Alloc_)(NH_INIT_SZ);
#endif

#ifdef NH_ROBIN_HOOD
  // Grow first if needed so Add_ never has to handle a full table.
  // (With RH at 75% load, the inner displacement loop can run
  // ~3 swaps on average; running it on a fully-loaded table would
  // never terminate.)
  if (NH_NEEDS_GROW(*pnh)) PRFX(NH_PFX,,Grow_)(pnh);
  return PRFX(NH_PFX,,Add_)(*pnh, key, old);
#else

  nh_t *nh = *pnh;
  uint32_t cap = nh->cap;
  uint32_t i = NH_HASH(key)&cap;
  NH_KEY *ks = nh->keys;

  if (old) *old = 1;

  for ( ; ks[i] != NH_KEY_NIL; i = (i+1)&cap) {
    if (NH_EQUAL(ks[i], key)) return nh->vals+i;
  }

  if (NH_NEEDS_GROW(*pnh)) {
    PRFX(NH_PFX,,Grow_)(pnh);
    nh = *pnh;
    cap = nh->cap;
    i = NH_HASH(key)&cap;
    ks = nh->keys;

    for ( ; ks[i] != NH_KEY_NIL; i = (i+1)&cap) {
      if (NH_EQUAL(ks[i], key)) return nh->vals+i;
    }

  }


  ks[i] = key;
  nh->used++;
  if (old) *old = 0;
#ifdef NH_INIT_VAL
  NH_INIT_VAL(nh->vals[i]);
#endif
  return nh->vals+i;
#endif // NH_ROBIN_HOOD
}



INLINE void PRFX(NH_PFX,,Set)(nh_t **nh, NH_KEY key, NH_VAL value) {
  *PRFX(NH_PFX,,Add)(nh, key, 0) = value;
}

INLINE void PRFX(NH_PFX,,Del)(nh_t **pnh, NH_KEY key) {
  nh_t *nh = *pnh;
#ifndef NH_PREP
  if (!nh) return;
#endif
  uint32_t cap = nh->cap;
  NH_KEY *ks = nh->keys;
  NH_VAL *vs = nh->vals;

#ifdef NH_ROBIN_HOOD
  // Robin Hood deletion: find the slot using RH early-exit, then
  // backshift the trailing chain to maintain the "no slot is more
  // displaced than its successor" invariant.
  uint32_t home_my = NH_HASH(key) & cap;
  uint32_t i = home_my;
  uint32_t dib_my = 0;
  for ( ; ks[i] != NH_KEY_NIL; i = (i+1)&cap, dib_my++) {
    if (NH_EQUAL(ks[i], key)) {
      // Found. Walk forward shifting each entry back by one slot
      // until we hit either an empty slot or an entry already at
      // its home (dib == 0).
      uint32_t j = i;
      for (;;) {
        uint32_t nxt = (j+1) & cap;
        if (ks[nxt] == NH_KEY_NIL) {
          ks[j] = NH_KEY_NIL;
          break;
        }
        uint32_t home_nxt = NH_HASH(ks[nxt]) & cap;
        if (nxt == home_nxt) {  // entry is at home; don't move
          ks[j] = NH_KEY_NIL;
          break;
        }
        ks[j] = ks[nxt];
        vs[j] = vs[nxt];
        j = nxt;
      }
      nh->used--;
      return;
    }
    uint32_t home_them = NH_HASH(ks[i]) & cap;
    uint32_t dib_them = (i - home_them) & cap;
    if (dib_them < dib_my) return; // RH invariant -> not in table
  }
  return; // empty chain
#else

  uint32_t i = NH_HASH(key)&cap;
  for ( ; ks[i] != NH_KEY_NIL; i = (i+1)&cap) {
    if (NH_EQUAL(ks[i], key)) {
      ks[i] = NH_KEY_NIL;
      nh->used--;
      i = (i+1)&cap;
      break;
    }
  }
  //rehash the following keys
  for ( ; ks[i] != NH_KEY_NIL; i = (i+1)&cap) {
    int old;
    NH_VAL *moved = PRFX(NH_PFX,,Add_)(nh, ks[i], &old);
    if (old) break; // all the following keys are reachable as is
    *moved = nh->vals[i];
    ks[i] = NH_KEY_NIL;
    nh->used--;
  }
#endif // NH_ROBIN_HOOD
}

typedef uint32_t PRFX(NH_PFX,,it_t);
#define nh_it_t PRFX(NH_PFX,,it_t)

INLINE nh_it_t PRFX(NH_PFX,,ValidKey)(nh_t *nh, nh_it_t i) {
  return nh->keys[i] != NH_KEY_NIL;
}

INLINE NH_KEY *PRFX(NH_PFX,,Key)(nh_t *nh, nh_it_t i) {
  return nh->keys+i;
}

INLINE NH_VAL *PRFX(NH_PFX,,Val)(nh_t *nh, nh_it_t i) {
  return nh->vals+i;
}


#ifndef NH_FOR
#define NH_FOR(prfx,i,nh) \
  for (PRFX(prfx,,it_t) i = 0, cap_=PRFX(prfx,,Cap)(nh); i!=cap_; i++) \
    if (PRFX(prfx,,ValidKey)(nh, i))
#endif



#undef nh_iter_t

#undef NH_KEY
#undef NH_VAL
#undef NH_KEY_NIL
#undef NH_PFX
#undef nh_t
#undef NH_INIT_SZ
#undef NH_NEEDS_GROW
#undef NH_HASH
#undef NH_EQUAL
#undef NH_ALLOC

#ifdef NH_INIT_VAL
#undef NH_INIT_VAL
#endif

#ifdef NH_PREP
#undef NH_PREP
#endif

#ifdef NH_ROBIN_HOOD
#undef NH_ROBIN_HOOD
#endif

#if 0

#define NH_VAL_NIL 0
#define NH_PREP
#include "nh.h"

void nhTest() {
  nh_t *nh = 0;
  for (int i = 'a'; i <= 'z'; i++) {
    nhSet(&nh,i,i-'a');
  }
  nhDel(&nh,'x');
  nhDel(&nh,'y');
  nhDel(&nh,'z');

  for (int i = 0; i < nhCap(nh); i++) {
    if (nh->keys[i]) printf("%c = %d\n", nh->keys[i], nh->vals[i]);
  }
  exit(0);
}

void nhTest2() {
  nh_t *h = nhAlloc();
  nhSet(&h,7,666);
  nhSet(&h,10000,479);
  nhSet(&h,13,123);
  NH_FOR(nh,i,h) printf("%d=%d\n", *nhKey(h,i), *nhVal(h,i));
  exit(0);
}


#endif

