// nh -- generic open-addressed hashmap (template header)
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
#define nh_slot_t PRFX(NH_PFX,,_slot_t)

// AM-pack-v2: pack {key, val, hash} into one struct
// per slot, inline in the nh_t tail. Hot-path hit reads ONE
// cache line for everything instead of three (keys[], vals[],
// hashes[] as separate arrays). Trade: per-slot footprint
// grows from ~20 bytes (split across three arrays) to 24
// bytes (one struct), but we drop two separate mallocs per
// table. AM-15's hash cache is still opt-in via
// NH_CACHE_HASH; without it the slot is 16 bytes (no hash
// field). The half-pack ({key, hash} only, vals separate)
// that AM-pack tried in 2438e33 lost on insert/del because
// the val store was still on a separate cache line; the
// full pack here keeps everything together.
typedef struct {
  NH_KEY key;
  NH_VAL val;
#ifdef NH_CACHE_HASH
  uint32_t hash;
  uint32_t _pad;
#endif
} nh_slot_t;

typedef struct {
  uint32_t cap; //capacity
  uint32_t used;
  nh_slot_t slots[1];  /* inline tail; cap+1 entries */
} nh_t;

/* Accessors. Centralised so the {key,val,hash} layout doesn't
 * leak into every loop body. */
#define NH_K(nh, i)    ((nh)->slots[i].key)
#define NH_V(nh, i)    ((nh)->slots[i].val)
#ifdef NH_CACHE_HASH
#define NH_H(nh, i)    ((nh)->slots[i].hash)
#endif
#define NH_VPTR(nh, i) (&(nh)->slots[i].val)

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

INLINE nh_t *PRFX(NH_PFX,,Alloc_)(int cap) {
  nh_t *nh = (nh_t*)NH_ALLOC(sizeof(nh_t)+sizeof(nh_slot_t)*(cap-1));
  /* Zero the entire slot array first; this initialises val
   * fields to all-zero (NH_ZERO_VALS semantics) and the hash
   * field (when present) to 0. The key field is then fixed
   * up below if NH_KEY_NIL isn't all-zero. */
  memset(nh->slots, 0, sizeof(nh_slot_t)*cap);
#ifdef NH_ZERO_VALS
#undef NH_ZERO_VALS
#endif

  nh->cap = cap-1;
  nh->used = 0;
  for (int i = 0; i < cap; i++) {
    nh->slots[i].key = NH_KEY_NIL;
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
  nh_slot_t *ss = nh->slots;
#ifdef NH_CACHE_HASH
  uint32_t hash_my = (uint32_t)NH_HASH(key);
  uint32_t home_my = hash_my & cap;
#else
  uint32_t home_my = NH_HASH(key) & cap;
#endif
  uint32_t i = home_my;
  uint32_t dib_my = 0;

  for ( ; ss[i].key != NH_KEY_NIL; i = (i+1)&cap, dib_my++) {
    if (NH_EQUAL(ss[i].key, key)) {
      if (old) *old = 1;
      return &ss[i].val;
    }
#ifdef NH_CACHE_HASH
    uint32_t hash_them = ss[i].hash;
    uint32_t home_them = hash_them & cap;
#else
    uint32_t home_them = NH_HASH(ss[i].key) & cap;
#endif
    uint32_t dib_them = (i - home_them) & cap;
    if (dib_them < dib_my) {
      // Robin Hood swap: install our key at this slot, take the
      // resident along on the chain. The new entry's val isn't
      // known yet (the caller writes through the returned pointer),
      // so we remember the slot and re-insert the displaced one.
      NH_KEY  k_disp = ss[i].key;
      NH_VAL  v_disp = ss[i].val;
#ifdef NH_CACHE_HASH
      uint32_t h_disp = hash_them;
      ss[i].hash = hash_my;
#endif
      ss[i].key = key;
      NH_VAL *ret = &ss[i].val;
#ifdef NH_INIT_VAL
      NH_INIT_VAL(*ret);
#endif
      nh->used++;
      if (old) *old = 0;

      // Inner loop: keep displacing until we hit an empty slot.
      uint32_t j  = (i+1) & cap;
      NH_KEY   k  = k_disp;
      NH_VAL   v  = v_disp;
#ifdef NH_CACHE_HASH
      uint32_t h  = h_disp;
#endif
      uint32_t home_j = home_them;
      uint32_t dib_j  = (j - home_j) & cap;
      for (;; j = (j+1)&cap, dib_j++) {
        if (ss[j].key == NH_KEY_NIL) {
          ss[j].key = k;
          ss[j].val = v;
#ifdef NH_CACHE_HASH
          ss[j].hash = h;
#endif
          return ret;
        }
#ifdef NH_CACHE_HASH
        uint32_t hash_jx = ss[j].hash;
        uint32_t home_jx = hash_jx & cap;
#else
        uint32_t home_jx = NH_HASH(ss[j].key) & cap;
#endif
        uint32_t dib_jx  = (j - home_jx) & cap;
        if (dib_jx < dib_j) {
          NH_KEY kt = ss[j].key; NH_VAL vt = ss[j].val;
          ss[j].key = k; ss[j].val = v;
          k = kt; v = vt;
#ifdef NH_CACHE_HASH
          { uint32_t ht = ss[j].hash; ss[j].hash = h; h = ht; }
#endif
          home_j = home_jx;
          dib_j  = dib_jx;
        }
      }
      // unreachable: the inner loop only exits via the empty slot.
    }
  }

  // Empty slot found before any Robin Hood swap.
  ss[i].key = key;
#ifdef NH_CACHE_HASH
  ss[i].hash = hash_my;
#endif
  nh->used++;
  if (old) *old = 0;
#ifdef NH_INIT_VAL
  NH_INIT_VAL(ss[i].val);
#endif
  return &ss[i].val;
}

INLINE NH_VAL *PRFX(NH_PFX,,Lookup)(nh_t *nh, NH_KEY key) {
#ifndef NH_PREP
  if (!nh) return 0;
#endif
  uint32_t cap = nh->cap;
  nh_slot_t *ss = nh->slots;
#ifdef NH_CACHE_HASH
  uint32_t home_my = (uint32_t)NH_HASH(key) & cap;
#else
  uint32_t home_my = NH_HASH(key) & cap;
#endif
  uint32_t i = home_my;
  uint32_t dib_my = 0;
  for ( ; ss[i].key != NH_KEY_NIL; i = (i+1)&cap, dib_my++) {
    if (NH_EQUAL(ss[i].key, key)) return &ss[i].val;
#ifdef NH_CACHE_HASH
    uint32_t home_them = ss[i].hash & cap;
#else
    uint32_t home_them = NH_HASH(ss[i].key) & cap;
#endif
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
  nh_slot_t *ss = nh->slots;
#ifdef NH_CACHE_HASH
  uint32_t home_my = (uint32_t)NH_HASH(key) & cap;
#else
  uint32_t home_my = NH_HASH(key) & cap;
#endif
  uint32_t i = home_my;
  uint32_t dib_my = 0;
  for ( ; ss[i].key != NH_KEY_NIL; i = (i+1)&cap, dib_my++) {
    if (NH_EQUAL(ss[i].key, key)) return ss[i].val;
#ifdef NH_CACHE_HASH
    uint32_t home_them = ss[i].hash & cap;
#else
    uint32_t home_them = NH_HASH(ss[i].key) & cap;
#endif
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
  nh_slot_t *ss = nh->slots;

  for ( ; ss[i].key != NH_KEY_NIL; i = (i+1)&cap) {
    if (NH_EQUAL(ss[i].key, key)) {
      if (old) *old = 1;
      return &ss[i].val;
    }
  }

  ss[i].key = key;
  nh->used++;
  if (old) *old = 0;
#ifdef NH_INIT_VAL
  NH_INIT_VAL(ss[i].val);
#endif
  return &ss[i].val;
}


INLINE NH_VAL *PRFX(NH_PFX,,Lookup)(nh_t *nh, NH_KEY key) {
#ifndef NH_PREP
  if (!nh) return 0;
#endif
  uint32_t cap = nh->cap;
  uint32_t i = NH_HASH(key)&cap;
  nh_slot_t *ss = nh->slots;
  for ( ; ss[i].key != NH_KEY_NIL; i = (i+1)&cap) {
    if (NH_EQUAL(ss[i].key, key)) return &ss[i].val;
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
  nh_slot_t *ss = nh->slots;
  for ( ; ss[i].key != NH_KEY_NIL; i = (i+1)&cap) {
    if (NH_EQUAL(ss[i].key, key)) return ss[i].val;
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
  nh_slot_t *oss = onh->slots;
  for (uint32_t i = 0; i < cap; i++) {
    if (oss[i].key == NH_KEY_NIL) continue;
    *PRFX(NH_PFX,,Add_)(nh, oss[i].key, 0) = oss[i].val;
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
  nh_slot_t *ss = nh->slots;

  if (old) *old = 1;

  for ( ; ss[i].key != NH_KEY_NIL; i = (i+1)&cap) {
    if (NH_EQUAL(ss[i].key, key)) return &ss[i].val;
  }

  if (NH_NEEDS_GROW(*pnh)) {
    PRFX(NH_PFX,,Grow_)(pnh);
    nh = *pnh;
    cap = nh->cap;
    i = NH_HASH(key)&cap;
    ss = nh->slots;

    for ( ; ss[i].key != NH_KEY_NIL; i = (i+1)&cap) {
      if (NH_EQUAL(ss[i].key, key)) return &ss[i].val;
    }

  }


  ss[i].key = key;
  nh->used++;
  if (old) *old = 0;
#ifdef NH_INIT_VAL
  NH_INIT_VAL(ss[i].val);
#endif
  return &ss[i].val;
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
  nh_slot_t *ss = nh->slots;

#ifdef NH_ROBIN_HOOD
  // Robin Hood deletion: find the slot using RH early-exit, then
  // backshift the trailing chain to maintain the "no slot is more
  // displaced than its successor" invariant.
#ifdef NH_CACHE_HASH
  uint32_t home_my = (uint32_t)NH_HASH(key) & cap;
#else
  uint32_t home_my = NH_HASH(key) & cap;
#endif
  uint32_t i = home_my;
  uint32_t dib_my = 0;
  for ( ; ss[i].key != NH_KEY_NIL; i = (i+1)&cap, dib_my++) {
    if (NH_EQUAL(ss[i].key, key)) {
      // Found. Walk forward shifting each entry back by one slot
      // until we hit either an empty slot or an entry already at
      // its home (dib == 0).
      uint32_t j = i;
      for (;;) {
        uint32_t nxt = (j+1) & cap;
        if (ss[nxt].key == NH_KEY_NIL) {
          ss[j].key = NH_KEY_NIL;
          break;
        }
#ifdef NH_CACHE_HASH
        uint32_t home_nxt = ss[nxt].hash & cap;
#else
        uint32_t home_nxt = NH_HASH(ss[nxt].key) & cap;
#endif
        if (nxt == home_nxt) {  // entry is at home; don't move
          ss[j].key = NH_KEY_NIL;
          break;
        }
        ss[j].key = ss[nxt].key;
        ss[j].val = ss[nxt].val;
#ifdef NH_CACHE_HASH
        ss[j].hash = ss[nxt].hash;
#endif
        j = nxt;
      }
      nh->used--;
      return;
    }
#ifdef NH_CACHE_HASH
    uint32_t home_them = ss[i].hash & cap;
#else
    uint32_t home_them = NH_HASH(ss[i].key) & cap;
#endif
    uint32_t dib_them = (i - home_them) & cap;
    if (dib_them < dib_my) return; // RH invariant -> not in table
  }
  return; // empty chain
#else

  uint32_t i = NH_HASH(key)&cap;
  for ( ; ss[i].key != NH_KEY_NIL; i = (i+1)&cap) {
    if (NH_EQUAL(ss[i].key, key)) {
      ss[i].key = NH_KEY_NIL;
      nh->used--;
      i = (i+1)&cap;
      break;
    }
  }
  //rehash the following keys
  for ( ; ss[i].key != NH_KEY_NIL; i = (i+1)&cap) {
    int old;
    NH_VAL *moved = PRFX(NH_PFX,,Add_)(nh, ss[i].key, &old);
    if (old) break; // all the following keys are reachable as is
    *moved = ss[i].val;
    ss[i].key = NH_KEY_NIL;
    nh->used--;
  }
#endif // NH_ROBIN_HOOD
}

typedef uint32_t PRFX(NH_PFX,,it_t);
#define nh_it_t PRFX(NH_PFX,,it_t)

INLINE nh_it_t PRFX(NH_PFX,,ValidKey)(nh_t *nh, nh_it_t i) {
  return nh->slots[i].key != NH_KEY_NIL;
}

INLINE NH_KEY *PRFX(NH_PFX,,Key)(nh_t *nh, nh_it_t i) {
  return &nh->slots[i].key;
}

INLINE NH_VAL *PRFX(NH_PFX,,Val)(nh_t *nh, nh_it_t i) {
  return &nh->slots[i].val;
}


#ifndef NH_FOR
#define NH_FOR(prfx,i,nh) \
  for (PRFX(prfx,,it_t) i = 0, cap_=PRFX(prfx,,Cap)(nh); i!=cap_; i++) \
    if (PRFX(prfx,,ValidKey)(nh, i))
#endif



#undef nh_iter_t
#undef nh_slot_t
#undef NH_K
#undef NH_V
#undef NH_VPTR
#ifdef NH_CACHE_HASH
#undef NH_H
#endif

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

#ifdef NH_CACHE_HASH
#undef NH_CACHE_HASH
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

