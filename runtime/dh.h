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

INLINE dyn dhEqual_(dyn a, dyn b) {
  GC_DISABLE();
  dyn r;
  ARGLIST2(a,b);
  MCALL(r,a,api.m_equal);
  GC_ENABLE();
  return r;
}

INLINE uint32_t dhHash_(dyn key) {
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
