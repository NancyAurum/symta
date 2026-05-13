/*****************************

  Adaptive Map

  An adaptive hash table whose internal representation specialises
  to the observed key/value distribution at runtime. One table
  object (an O_TAG=T_TBL heap allocation, 2 cells: base + void)
  can be in any of six modes:

    AM_EMPTY    - no backing; everything misses
    AM_BITMAP0  - bitmap of present integer keys; values are
                  implicitly FXN(0). Set bit -> get returns 0.
    AM_BITMAP1  - bitmap; values implicitly FXN(1).
    AM_INT      - stb_ds hashmap, integer-keyed.
    AM_TEXT     - stb_ds hashmap, string-keyed.
    AM_GENERIC  - nh_t-derived dh_t hashmap, dyn-keyed
                  (any mix of types).

  Mode transitions happen lazily inside amSet/amGidSet: every
  promotion path widens the underlying type (the inverse demotion
  doesn't exist -- once promoted to AM_INT, a column that drains
  back to all-zero values stays in INT mode until amClear).

  --- void_val contract ---

  AM_VOID(o) holds the value returned for missing keys. Defaults
  to No (set in amNew). The user can change it with `T.setNo X`
  (-> amSetNo).

  amSet / amGidSet treat `value == void_val` as a delete request.
  In particular, this means:
    - Default void_val = No: `T.K = No` deletes K. (Expected.)
    - After `T.setNo 0`: `T.K = 0` also deletes K, even though
      the user "stored a value". This is the AM-7 quirk -- the
      surprise is greatest on BITMAP0 tables, where 0 is *also*
      the implicit stored value. We have no separate "remove key"
      API in Symta today; the convention is to either pick a
      void_val that's not in your value alphabet (the default No
      is usually right), or use T.del K explicitly.

  --- AM_EMPTY initial-mode selection asymmetry ---

  amGidSet (component-column setter, used by cls fields) and
  amSet (user-facing T.K=V) pick different initial modes for an
  empty table when the first value is FXN(0):

    amGidSet, value == FXN(0) -> AM_BITMAP0
    amSet,    value == FXN(0) -> AM_INT

  Same observable behaviour (`T.has K` returns 1, `T.K` returns
  0 in both); different memory cost. amSet's choice optimises
  for the common case where T.K=0 is a real stored value rather
  than a membership marker, accepting the AM_INT footprint.
  ECS-style component columns go through amGidSet, which picks
  BITMAP0 for the dense-zero case. amSet matches amGidSet for
  FXN(1) (both pick AM_BITMAP1) -- the asymmetry is only on 0.

  --- iteration / mutation ---

  amL and amKs eagerly build a fresh list of the current
  contents; mutation after the call returns doesn't disturb the
  returned list. amL therefore *implicitly snapshots*; we don't
  need an explicit "iteration in progress" guard. Callers that
  walk the table via NH_FOR/internal pointers (e.g. from inside
  a finalizer) are on their own.

  --- references ---

  See TODO.md (AM-* items) for the open polish backlog.

Todo:
* Efficient strings set representation:
  Use a default value 1 or a radix judy tree instead of a string-keyed hash map.


******************************/


#ifndef AM_H
#define AM_H

#include "ng.h"
#include "dh.h"
#include "nb.h"
#include "prf.h"

#ifdef AM_IMPLEMENTATION
uint32_t amFinalizerHook;
#undef AM_IMPLEMENTATION
#else
extern uint32_t amFinalizerHook;
#endif


#define AM_ALLOC(r) OBJECT(r, T_TBL, 2)

#define AM_BASE(o) (*((void **)&LGET8((o),0)))
#define AM_VOID(o) (*((void **)&LGET8((o),1)))

#define AM_TYPE(o) (O_CODE(o)&0xFF)
#define AM_SET_TYPE(o,v) O_CODE(o) = ((O_CODE(o)&~(uint32_t)0xFF)|(v))

//the age of the youngest object it holds
#define AM_YOUNGEST(o) (O_CODE(o)>>8)
#define AM_SET_YOUNGEST(o,v) \
  O_CODE(o) = ((O_CODE(o)&~(uint32_t)0xFFFF00)|((v)<<8))


#define AM_OLDER(hm,v) (!IMMEDIATE(v) && AM_YOUNGEST(hm) > O_AGE(v))

#define AM_ATTRACT(hm,v)                \
  if (AM_OLDER(hm, v)) {                \
    int v_age = O_AGE(v);               \
    AM_SET_YOUNGEST(hm,v_age);          \
    arrput(api.hg0[v_age].magnets, hm); \
  }

#define AM_EMPTY   0
#define AM_GENERIC 1
#define AM_TEXT    2
#define AM_INT     3
#define AM_BITMAP0 4
#define AM_BITMAP1 5


typedef struct { uint64_t key; dyn value; } *symta_itbl;
typedef struct { char *key; dyn value; } *symta_stbl;


INLINE dyn amGidGet(dyn o, dyn ref) {
  dyn r;
  CASE(AM_TYPE(o))
  GOT(AM_EMPTY) r = AM_VOID(o);
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    dyn key = FXN(O_GID(ref));
    r = hmget(hm,(uint64_t)key);
    if (r == NIL) r = AM_VOID(o);
  GOT(AM_BITMAP0)
    nb_t nb = AM_BASE(o);
    dyn key = FXN(O_GID(ref));
    r = nbGet(nb, UNFXN(key)) ? FXN(0) : AM_VOID(o);
  GOT(AM_BITMAP1)
    nb_t nb = AM_BASE(o);
    dyn key = FXN(O_GID(ref));
    r = nbGet(nb, UNFXN(key)) ? FXN(1) : AM_VOID(o);
  GOT(AM_GENERIC)
    /* Column promoted to GENERIC at some point (mixed key types or
     * a non-int key got written). The keys are stored as full Symta
     * values; look up by the gid-as-int key the same way amGet would.
     * AM-1 (TODO.md): fixes the latent UB where amGidGet returned
     * `r` uninitialised on this branch. */
    dh_t *hm = AM_BASE(o);
    r = dhGet(hm, FXN(O_GID(ref)));
    if (r == NIL) r = AM_VOID(o);
  GOT(AM_TEXT)
    /* Text-keyed column being queried by entity GID is a contract
     * violation -- cls fields are integer-keyed by construction.
     * Crash loudly so the bug surfaces at the call site instead of
     * silently returning garbage. (Also AM-1 / TODO.md.) */
    fatal("amGidGet: AM_TEXT column queried by integer GID\n");
    r = AM_VOID(o); /* unreachable; keeps the compiler quiet */
  ESAC
  return r;
}

INLINE dyn amRefs(dyn o, uint64_t tag) {
  dyn r;
  GC_DISABLE();
  CASE(AM_TYPE(o))
  GOT(AM_EMPTY)
    r = Empty;
  GOT(AM_TEXT)
    r = Empty;
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    int n = hmlen(hm);
    LIST(r,n);
    for (int i=0; i < n; ++i) {
      LGET(r,i) = MKIMM(tag,UNFXN((dyn)hm[i].key));
    }
  GOT(AM_GENERIC)
    r = Empty;
  GOT(AM_BITMAP0)
    nb_t nb = AM_BASE(o);
    int n = nbN(nb);
    LIST(r,n);
    int i = 0;
    NB_FOR(id,nb) {
      LGET(r,i) = MKIMM(tag,(uint64_t)id);
      ++i;
    }
  GOT(AM_BITMAP1)
    nb_t nb = AM_BASE(o);
    int n = nbN(nb);
    LIST(r,n);
    int i = 0;
    NB_FOR(id,nb) {
      LGET(r,i) = MKIMM(tag,(uint64_t)id);
      ++i;
    }
  ESAC
  GC_ENABLE();
  return r;
}

INLINE void amGidSet(dyn o, dyn ref, dyn value) {
  AM_ATTRACT(o,value);
  dyn key = FXN(O_GID(ref));
  dyn void_val = AM_VOID(o);
  if (value == void_val) { //we are allowed to delete these
    CASE(AM_TYPE(o))
    GOT(AM_EMPTY)
      return;
    GOT(AM_GENERIC)
      dh_t *hm = AM_BASE(o);
      dhDel(&hm, key);
      AM_BASE(o) = hm;
      return;
    GOT(AM_INT)
      symta_itbl hm = AM_BASE(o);
      if (O_TAG(key) == T_INT) {
        hmdel(hm, (uint64_t)key);
        AM_BASE(o) = hm;
      }
      return;
    GOT(AM_BITMAP0)
      if (O_TAG(key) == T_INT) {
        nb_t nb = AM_BASE(o);
        nbDel(&nb, UNFXN(key));
        AM_BASE(o) = nb;
      }
      return;
    GOT(AM_BITMAP1)
      if (O_TAG(key) == T_INT) {
        nb_t nb = AM_BASE(o);
        nbDel(&nb, UNFXN(key));
        AM_BASE(o) = nb;
      }
      return;
    ESAC
  }


  CASE(AM_TYPE(o))
  GOT(AM_EMPTY)
    if (value == FXN(0)) {
      nb_t nb = nbAlloc();
      nbSet(&nb,UNFXN(key));
      AM_BASE(o) = nb;
      AM_SET_TYPE(o, AM_BITMAP0);
    } else if (value == FXN(1)) {
      nb_t nb = nbAlloc();
      nbSet(&nb,UNFXN(key));
      AM_BASE(o) = nb;
      AM_SET_TYPE(o, AM_BITMAP1);
    } else {
      symta_itbl hm = 0;
      hmdefault(hm, NIL);
      hmput(hm, (uint64_t)key, value);
      AM_BASE(o) = hm;
      AM_SET_TYPE(o, AM_INT);
    }
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    hmput(hm, (uint64_t)key, value);
    AM_BASE(o) = hm;
  GOT(AM_BITMAP0)
    nb_t nb = AM_BASE(o);
    if (value == FXN(0)) {
      nbSet(&nb,UNFXN(key));
      AM_BASE(o) = nb;
    } else {
      symta_itbl hm = 0;
      hmdefault(hm, NIL);
      NB_FOR(id,nb) hmput(hm, (uint64_t)FXN(id), FXN(0));
      hmput(hm, (uint64_t)key, value);
      AM_BASE(o) = hm;
      AM_SET_TYPE(o, AM_INT);
    }
  GOT(AM_BITMAP1)
    nb_t nb = AM_BASE(o);
    if (value == FXN(1)) {
      nbSet(&nb,UNFXN(key));
      AM_BASE(o) = nb;
    } else {
      symta_itbl hm = 0;
      hmdefault(hm, NIL);
      NB_FOR(id,nb) hmput(hm, (uint64_t)FXN(id), FXN(1));
      hmput(hm, (uint64_t)key, value);
      AM_BASE(o) = hm;
      AM_SET_TYPE(o, AM_INT);
    }
  ESAC
}

static dyn amClear(uint8_t *bc_) {
  dyn o = LGET(api.args,0);
  CASE(AM_TYPE(o))
  GOT(AM_EMPTY)
  GOT(AM_GENERIC)
    dh_t *hm = AM_BASE(o);
    dhFree(hm);
  GOT(AM_TEXT)
    symta_stbl hm = AM_BASE(o);
    shfree(hm);
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    hmfree(hm);
  GOT(AM_BITMAP0)
    nb_t nb = AM_BASE(o);
    nbFree(nb);
  GOT(AM_BITMAP1)
    nb_t nb = AM_BASE(o);
    nbFree(nb);
  ESAC
  AM_BASE(o) = 0;
  AM_SET_TYPE(o, AM_EMPTY);
  return 0;
}

INLINE dyn amNew() {
  dyn r;
  GC_DISABLE();
  AM_ALLOC(r);
  AM_VOID(r) = No;
  AM_SET_YOUNGEST(r,api.hgp->age);
  dyn fin;
  if (!amFinalizerHook) amFinalizerHook = sbc_hook(amClear,0);
  CLOSURE(fin, amFinalizerHook, 0);
  gc_set_finalizer(r, fin); //FIXME: this should be a builtin function
  GC_ENABLE();
  return r;
}

INLINE dyn amHas(dyn o, dyn key) { //Key exists
  dyn r;
  CASE(AM_TYPE(o))
  GOT(AM_EMPTY) return 0;
  GOT(AM_TEXT)
    if (!IS_TEXT(key)) return 0;
    symta_stbl hm = AM_BASE(o);
    r = shget(hm,text_chars(key));
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    r = hmget(hm,(uint64_t)key);
  GOT(AM_GENERIC)
    dh_t *hm = AM_BASE(o);
    r = dhGet(hm, key);
  GOT(AM_BITMAP0)
    nb_t nb = AM_BASE(o);
    return FXN(nbGet(nb, UNFXN(key)) == 0);
  GOT(AM_BITMAP1)
    nb_t nb = AM_BASE(o);
    return FXN(nbGet(nb, UNFXN(key)) != 0);
  ESAC
  return FXN(r != NIL);
}

INLINE dyn amGot(dyn o, dyn key) { //key both exists and has value != No
  dyn r;
  CASE(AM_TYPE(o))
  GOT(AM_EMPTY) return 0;
  GOT(AM_TEXT)
    if (!IS_TEXT(key)) return 0;
    symta_stbl hm = AM_BASE(o);
    r = shget(hm,text_chars(key));
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    r = hmget(hm,(uint64_t)key);
  GOT(AM_GENERIC)
    dh_t *hm = AM_BASE(o);
    r = dhGet(hm, key);
  GOT(AM_BITMAP0)
    nb_t nb = AM_BASE(o);
    int64_t rr = nbGet(nb, UNFXN(key)) == 0;
    return FXN(rr);
  GOT(AM_BITMAP1)
    nb_t nb = AM_BASE(o);
    int64_t rr = nbGet(nb, UNFXN(key)) != 0;
    return FXN(rr);
  ESAC
  return FXN(r != NIL && r != No);
}

INLINE dyn amGet(dyn o, dyn key) {
  dyn r;
  CASE(AM_TYPE(o))
  GOT(AM_EMPTY) return AM_VOID(o);
  GOT(AM_TEXT)
    if (!IS_TEXT(key)) return AM_VOID(o);
    symta_stbl hm = AM_BASE(o);
    r = shget(hm,text_chars(key));
    if (r == NIL) r = AM_VOID(o);
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    r = hmget(hm,(uint64_t)key);
    if (r == NIL) r = AM_VOID(o);
  GOT(AM_GENERIC)
    dh_t *hm = AM_BASE(o);
    r = dhGet(hm, key);
    if (r == NIL) r = AM_VOID(o);
  GOT(AM_BITMAP0)
    nb_t nb = AM_BASE(o);
    r = nbGet(nb, UNFXN(key)) ? FXN(0) : AM_VOID(o);
  GOT(AM_BITMAP1)
    nb_t nb = AM_BASE(o);
    r = nbGet(nb, UNFXN(key)) ? FXN(1) : AM_VOID(o);
  ESAC
  return r;
}

INLINE void amSetNo(dyn o, dyn value) {
  AM_VOID(o) = value;
}


INLINE void amSwap(dyn o, dyn m) {
  dyn obase = AM_BASE(o);
  dyn ovoid = AM_VOID(o);
  uint32_t otype = AM_TYPE(o);
  uint32_t oyoungest = AM_YOUNGEST(o);

  AM_BASE(o) = AM_BASE(m);
  AM_VOID(o) = AM_VOID(m);
  AM_SET_TYPE(o,AM_TYPE(m));
  AM_SET_YOUNGEST(o,AM_YOUNGEST(m));

  AM_BASE(m) = obase;
  AM_VOID(m) = ovoid;
  AM_SET_TYPE(m,otype);
  AM_SET_YOUNGEST(m,oyoungest);
}

//FIXME: maps upgraded to a generic map can be
//       replaced with a special type on the next GC.
//       such type won't need a switch.
INLINE void amSet(dyn o, dyn key, dyn value) {
  dyn void_val = AM_VOID(o);
  if (value == void_val) { //we are allowed to delete these
    CASE(AM_TYPE(o))
    GOT(AM_EMPTY)
       return;
    GOT(AM_GENERIC)
      dh_t *hm = AM_BASE(o);
      dhDel(&hm, key);
      AM_BASE(o) = hm;
      return;
    GOT(AM_TEXT)
      symta_stbl hm = AM_BASE(o);
      if (IS_TEXT(key)) {
        shdel(hm, text_chars(key));
        AM_BASE(o) = hm;
      }
      /* AM-11 (TODO.md): without this `return`, control falls out
       * of the delete-branch switch, past the `if (value==void_val)`
       * block, and into the regular AM_TEXT insert branch below --
       * which `shput`s the key right back. T.K = void_val on a
       * text-keyed table looked like a no-op. */
      return;
    GOT(AM_INT)
      symta_itbl hm = AM_BASE(o);
      if (O_TAG(key) == T_INT) {
        hmdel(hm, (uint64_t)key);
        AM_BASE(o) = hm;
      }
      return;
    GOT(AM_BITMAP0)
      if (O_TAG(key) == T_INT) {
        nb_t nb = AM_BASE(o);
        nbDel(&nb, UNFXN(key));
        AM_BASE(o) = nb;
      }
      return;
    GOT(AM_BITMAP1)
      if (O_TAG(key) == T_INT) {
        nb_t nb = AM_BASE(o);
        nbDel(&nb, UNFXN(key));
        AM_BASE(o) = nb;
      }
      return;
    ESAC
  }

  AM_ATTRACT(o,key);
  AM_ATTRACT(o,value);

  CASE(AM_TYPE(o))
  GOT(AM_EMPTY)
    //pick initial type
    if (IS_TEXT(key)) {
      symta_stbl hm = 0;
      sh_new_arena(hm); //make shput copy the text chars
      shdefault(hm, NIL);
      shput(hm, text_chars(key), value);
      AM_BASE(o) = hm;
      AM_SET_TYPE(o, AM_TEXT);
    } else if (O_TAG(key) == T_INT) {
      if (value == FXN(1)) {
        nb_t nb = nbAlloc();
        nbSet(&nb,UNFXN(key));
        AM_BASE(o) = nb;
        AM_SET_TYPE(o, AM_BITMAP1);
      } else {
        symta_itbl hm = 0;
        hmdefault(hm, NIL);
        hmput(hm, (uint64_t)key, value);
        AM_BASE(o) = hm;
        AM_SET_TYPE(o, AM_INT);
      }
    } else {
      dh_t *hm = dhAlloc();
      dhSet(&hm, key, value);
      AM_BASE(o) = hm;
      AM_SET_TYPE(o, AM_GENERIC);
    }
  GOT(AM_GENERIC)
    dh_t *hm = AM_BASE(o);
    dhSet(&hm, key, value);
    AM_BASE(o) = hm;
  GOT(AM_TEXT)
    symta_stbl hm = AM_BASE(o);
    if (IS_TEXT(key)) {
      shput(hm, text_chars(key), value);
      AM_BASE(o) = hm;
    } else { //convert to dynamic key type hash map
      //fprintf(stderr, "cvt: %s\n", print_object(key));
      GC_DISABLE();
      dh_t *dh = dhAlloc();
      for (int i=0; i < shlen(hm); ++i) {
        dyn k;
        TEXT(k,hm[i].key);
        AM_ATTRACT(o,k); //values don't need that
        dhSet(&dh, k, hm[i].value);
      }
      GC_ENABLE();
      dhSet(&dh, key, value);
      AM_BASE(o) = dh;
      AM_SET_TYPE(o, AM_GENERIC);
      shfree(hm);
    }
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    if (O_TAG(key) == T_INT) {
      hmput(hm, (uint64_t)key, value);
      AM_BASE(o) = hm;
    } else { //convert to dynamic key type hash map
      dh_t *dh = dhAlloc();
      for (int i=0; i < hmlen(hm); ++i) {
        dhSet(&dh, (dyn)hm[i].key, hm[i].value);
      }
      dhSet(&dh, key, value);
      AM_BASE(o) = dh;
      AM_SET_TYPE(o, AM_GENERIC);
      hmfree(hm);
    }
  GOT(AM_BITMAP0)
    nb_t nb = AM_BASE(o);
    if (O_TAG(key) == T_INT) {
      if (value == FXN(0)) {
        nbSet(&nb,UNFXN(key));
        AM_BASE(o) = nb;
      } else {
        symta_itbl hm = 0;
        hmdefault(hm, NIL);
        NB_FOR(id,nb) hmput(hm, (uint64_t)FXN(id), FXN(0));
        hmput(hm, (uint64_t)key, value);
        AM_BASE(o) = hm;
        AM_SET_TYPE(o, AM_INT);
      }
    } else {
      dh_t *dh = dhAlloc();
      NB_FOR(id,nb) dhSet(&dh, FXN(id), FXN(0));
      dhSet(&dh, key, value);
      AM_BASE(o) = dh;
      AM_SET_TYPE(o, AM_GENERIC);
      nbFree(nb);
    }
  GOT(AM_BITMAP1)
    nb_t nb = AM_BASE(o);
    if (O_TAG(key) == T_INT) {
      if (value == FXN(1)) {
        nbSet(&nb,UNFXN(key));
        AM_BASE(o) = nb;
      } else {
        symta_itbl hm = 0;
        hmdefault(hm, NIL);
        NB_FOR(id,nb) hmput(hm, (uint64_t)FXN(id), FXN(1));
        hmput(hm, (uint64_t)key, value);
        AM_BASE(o) = hm;
        AM_SET_TYPE(o, AM_INT);
      }
    } else {
      dh_t *dh = dhAlloc();
      NB_FOR(id,nb) dhSet(&dh, FXN(id), FXN(1));
      dhSet(&dh, key, value);
      AM_BASE(o) = dh;
      AM_SET_TYPE(o, AM_GENERIC);
      nbFree(nb);
    }
  ESAC
}

INLINE dyn amDel(dyn o, dyn key) {
  CASE(AM_TYPE(o))
  GOT(AM_EMPTY)
  GOT(AM_GENERIC)
    dh_t *hm = AM_BASE(o);
    dhDel(&hm, key);
    AM_BASE(o) = hm;
  GOT(AM_TEXT)
    symta_stbl hm = AM_BASE(o);
    if (IS_TEXT(key)) {
      shdel(hm, text_chars(key));
      AM_BASE(o) = hm;
    }
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    if (O_TAG(key) == T_INT) {
      hmdel(hm, (uint64_t)key);
      AM_BASE(o) = hm;
    }
  GOT(AM_BITMAP0)
    if (O_TAG(key) == T_INT) {
      nb_t nb = AM_BASE(o);
      nbDel(&nb, UNFXN(key));
      AM_BASE(o) = nb;
    }
  GOT(AM_BITMAP1)
    if (O_TAG(key) == T_INT) {
      nb_t nb = AM_BASE(o);
      nbDel(&nb, UNFXN(key));
      AM_BASE(o) = nb;
    }
  ESAC
  return o;
}

INLINE dyn amN(dyn o) {
  CASE(AM_TYPE(o))
  GOT(AM_EMPTY)
  GOT(AM_GENERIC)
    dh_t *hm = AM_BASE(o);
    return FXN(dhN(hm));
  GOT(AM_TEXT)
    symta_stbl hm = AM_BASE(o);
    return FXN(shlen(hm));
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    return FXN(hmlen(hm));
  GOT(AM_BITMAP0)
    nb_t nb = AM_BASE(o);
    return FXN(nbN(nb));
  GOT(AM_BITMAP1)
    nb_t nb = AM_BASE(o);
    return FXN(nbN(nb));
  ESAC
  return FXN(0);
}

INLINE dyn amL(dyn o) {
  dyn r;
  GC_DISABLE();
  CASE(AM_TYPE(o))
  GOT(AM_EMPTY)
    r = Empty;
  GOT(AM_TEXT)
    symta_stbl hm = AM_BASE(o);
    int n = shlen(hm);
    LIST(r,n);
    for (int i=0; i < n; ++i) {
      dyn t, kv;
      LIST(kv,2);
      TEXT(t,hm[i].key);
      LGET(kv,0) = t;
      LGET(kv,1) = hm[i].value;
      LGET(r,i) = kv;
    }
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    int n = hmlen(hm);
    LIST(r,n);
    for (int i=0; i < n; ++i) {
      dyn kv;
      LIST(kv,2);
      LGET(kv,0) = (dyn)hm[i].key;
      LGET(kv,1) = hm[i].value;
      LGET(r,i) = kv;
    }
  GOT(AM_GENERIC)
    dh_t *hm = AM_BASE(o);
    dyn *keys = hm->keys;
    dyn *vals = hm->vals;
    LIST(r,dhN(hm));
    int j = 0;
    NH_FOR(dh,i,hm) {
      dyn kv;
      LIST(kv,2);
      LGET(kv,0) = *dhKey(hm,i);
      LGET(kv,1) = *dhVal(hm,i);
      LGET(r,j) = kv;
      j++;
    }
  GOT(AM_BITMAP0)
    nb_t nb = AM_BASE(o);
    int n = nbN(nb);
    LIST(r,n);
    int i = 0;
    NB_FOR(id,nb) {
      dyn kv;
      LIST(kv,2);
      LGET(kv,0) = FXN(id);
      LGET(kv,1) = FXN(0);
      LGET(r,i) = kv;
      ++i;
    }
  GOT(AM_BITMAP1)
    nb_t nb = AM_BASE(o);
    int n = nbN(nb);
    LIST(r,n);
    int i = 0;
    NB_FOR(id,nb) {
      dyn kv;
      LIST(kv,2);
      LGET(kv,0) = FXN(id);
      LGET(kv,1) = FXN(1);
      LGET(r,i) = kv;
      ++i;
    }
  ESAC
  GC_ENABLE();
  return r;
}

INLINE dyn amKs(dyn o) {
  dyn r;
  GC_DISABLE();
  CASE(AM_TYPE(o))
  GOT(AM_EMPTY)
    r = Empty;
  GOT(AM_TEXT)
    symta_stbl hm = AM_BASE(o);
    int n = shlen(hm);
    LIST(r,n);
    for (int i=0; i < n; ++i) {
      dyn t;
      TEXT(t,hm[i].key);
      LGET(r,i) = t;
    }
  GOT(AM_INT)
    symta_itbl hm = AM_BASE(o);
    int n = hmlen(hm);
    LIST(r,n);
    for (int i=0; i < n; ++i) {
      LGET(r,i) = (dyn)hm[i].key;
    }
  GOT(AM_GENERIC)
    dh_t *hm = AM_BASE(o);
    dyn *keys = hm->keys;
    LIST(r,dhN(hm));
    int j = 0;
    NH_FOR(dh,i,hm) {
      LGET(r,j) = *dhKey(hm,i);
      j++;
    }
  GOT(AM_BITMAP0)
    nb_t nb = AM_BASE(o);
    int n = nbN(nb);
    LIST(r,n);
    int i = 0;
    NB_FOR(id,nb) {
      LGET(r,i) = FXN(id);
      ++i;
    }
  GOT(AM_BITMAP1)
    nb_t nb = AM_BASE(o);
    int n = nbN(nb);
    LIST(r,n);
    int i = 0;
    NB_FOR(id,nb) {
      LGET(r,i) = FXN(id);
      ++i;
    }
  ESAC
  GC_ENABLE();
  return r;
}



#endif
