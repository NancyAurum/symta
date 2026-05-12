//Macros to create a growing hash table holding keys and elements of any type
//Copyright (c) 2020 Nash Gold
//All rights reserved.

#ifndef NCU_HSH_H
#define NCU_HSH_H

#include "base.h"

//initial table size
#define HSH_INIT_SZ   (1<<2)

//determines when the table needs to grow
#define HSH_NEEDS_GROW(hsh) hsh->used >= ((hsh->size+1)>>2)*3
//#define HSH_NEEDS_GROW(hsh) hsh->used >= ((hsh->size+1)>>1)
//#define HSH_NEEDS_GROW(hsh) hsh->used >= ((hsh->size+1)>>2)


//setting hashtable to below on init, removes the need to call <name>Init
#define HSH_INIT {0,0,0,0}


//FIXME: allow user to seed the table with random number.


/*
// a fast alternative to modulo:
// http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
uint32_t reduce(uint32_t x, uint32_t N) {
  return ((uint64_t) x * (uint64_t) N) >> 32 ;
}

uint32_t fast_range_probing(uint32_t hashed_key, uint32_t probe, uint32_t N)
{
  return ((uint64_t)hashed_key + ((uint64_t)probe << 32)/N) * N >> 32;
}

*/

#if 0
#include "jenkins.h"
NCU_INLINE uint32_t hsh_hfn(void *in, uint32_t len) {
  return jenkins_lookup3(in,strlen((char*)in),0xdeadbeef);
}
#else
#include "murmur.h"
NCU_INLINE uint32_t hsh_hfn(void *in, uint32_t len) {
  return PMurHash32(0xdeadbeef, in, len);
}
#endif

NCU_INLINE uint32_t hsh_cstr(void *in, uint32_t sz) {
  return hsh_hfn(in,strlen((char*)in))&sz;
}

NCU_INLINE uint32_t hsh_int(int in, uint32_t sz) {
  return hsh_hfn(&in, 4)&sz;
}

#define hsh_cmp_int(a,b) ((a)==(b))
#define hsh_nil_int INT32_MIN
#define hsh_valid_int(k) ((k)!=hsh_nil_int)

#define hsh_cmp_cstr(a,b) (!strcmp((a),(b)))
#define hsh_nil_cstr 0
#define hsh_valid_cstr(k) (k)


NCU_INLINE void *hsh_zmalloc_(size_t size) {
  void *p = malloc(size);
  memset(p, 0, size);
  return p;
}

#define hsh_dcl(type,hfn,key_type,elt_type) \
typedef struct {                            \
  key_type *keys;                           \
  uint32_t size;                            \
  uint32_t used;                            \
  elt_type *elts;                           \
} type;                                     \
NCU_INLINE key_type *type##kmalloc_(uint32_t size) {       \
  key_type *p = (key_type*)malloc(sizeof(key_type)*size);  \
  for (uint32_t i = 0; i < size; i+=4) {                   \
    p[i  ] = hsh_nil_##hfn;                                \
    p[i+1] = hsh_nil_##hfn;                                \
    p[i+2] = hsh_nil_##hfn;                                \
    p[i+3] = hsh_nil_##hfn;                                \
  }                                                        \
  return p;                                                \
}                                                          \
NCU_INLINE void type##Init(type *hsh) {                             \
  hsh->used = 0;                                                    \
  hsh->size = HSH_INIT_SZ-1;                                        \
  hsh->keys = type##kmalloc_(HSH_INIT_SZ);                          \
  hsh->elts = (elt_type *)hsh_zmalloc_(sizeof(hsh->elts[0])*HSH_INIT_SZ); \
}                                                                   \
NCU_INLINE void type##Free(type *hsh) { \
  if (!hsh->size) return;               \
  free(hsh->keys);                      \
  free(hsh->elts);                      \
  hsh->size = 0;                        \
  hsh->used = 0;                        \
}                                       \
NCU_INLINE elt_type *type##Get(type *hsh, key_type key) { \
  uint32_t sz = hsh->size;                               \
  if (!sz) return 0;                                     \
  uint32_t i = hsh_##hfn(key,sz);                        \
  for ( ; hsh_valid_##hfn(hsh->keys[i]); i = (i+1)&sz) { \
    if (hsh_cmp_##hfn(hsh->keys[i],key)) {               \
      return &hsh->elts[i];                              \
    }                                                    \
  }                                                      \
  return 0;                                              \
}                                                        \
NCU_INLINE elt_type *type##Add_(type *hsh, key_type key, int *old) { \
  uint32_t sz = hsh->size;                       \
  uint32_t i = hsh_##hfn(key,sz);                \
  for ( ; ; i = (i+1)&sz) {                      \
    if (!hsh_valid_##hfn(hsh->keys[i])) {        \
      hsh->keys[i] = key;                        \
      hsh->used++;                               \
      if (old) *old = 0;                         \
      return &hsh->elts[i];                      \
    }                                            \
    if (hsh_cmp_##hfn(hsh->keys[i],key)) {       \
      if (old) *old = 1;                         \
      return &hsh->elts[i];                      \
    }                                            \
  }                                              \
  return 0;                                      \
}                                                \
static void type##Grow(type *hsh) {                         \
  if (!hsh->size) {                                         \
    type##Init(hsh);                                        \
    return;                                                 \
  }                                                         \
  uint32_t size = hsh->size+1;                              \
  uint32_t new_size = size*2;                               \
  key_type *okeys = hsh->keys;                              \
  elt_type *oelts = hsh->elts;                              \
  hsh->keys = type##kmalloc_(new_size);                     \
  hsh->elts = hsh_zmalloc_(sizeof(hsh->elts[0])*new_size);  \
  hsh->size = new_size-1;                                   \
  hsh->used = 0;                                            \
  for (int i = 0; i < size; i++) {                          \
    if (!hsh_valid_##hfn(okeys[i])) continue;               \
    *type##Add_(hsh,okeys[i],0) = oelts[i];                 \
  }                                                         \
  free(okeys);                                              \
  free(oelts);                                              \
}                                                           \
NCU_INLINE elt_type *type##Add(type *hsh, key_type key, int *old) { \
  if (HSH_NEEDS_GROW(hsh)) type##Grow(hsh);             \
  return type##Add_(hsh,key,old);                       \
}                                                       \
NCU_INLINE int type##Collides(type *hsh, key_type key) { \
  uint32_t sz = hsh->size;                               \
  if (!sz) return 0;                                     \
  uint32_t i = hsh_##hfn(key,sz);                        \
  if (!hsh_valid_##hfn(hsh->keys[i])) return 0;          \
  if (hsh_cmp_##hfn(hsh->keys[i],key)) return 0;         \
  int collisions = 0;                                    \
  for ( ; hsh_valid_##hfn(hsh->keys[i]); i = (i+1)&sz) { \
    if (hsh_cmp_##hfn(hsh->keys[i],key)) break;          \
    else collisions++;                                   \
  }                                                      \
  return collisions;                                     \
}                                                        \
NCU_INLINE void type##Erase(type *hsh, key_type key) {  \
  uint32_t sz = hsh->size;                              \
  if (!sz) return;                                      \
  uint32_t i = hsh_##hfn(key,sz);                       \
  for ( ; hsh_valid_##hfn(hsh->keys[i]); i = (i+1)&sz) {\
    if (hsh_cmp_##hfn(hsh->keys[i],key)) {              \
      hsh->keys[i] = hsh_nil_##hfn;                     \
      hsh->used--;                                      \
      i = (i+1)&sz;                                     \
      break;                                            \
    }                                                   \
  }                                                     \
  for ( ; hsh_valid_##hfn(hsh->keys[i]); i = (i+1)&sz) {\
    int old;                                            \
    elt_type *r = type##Add_(hsh,hsh->keys[i],&old);    \
    if (r && !old) {                                    \
      *r = hsh->elts[i];                                \
      hsh->keys[i] = hsh_nil_##hfn;                     \
      hsh->used--;                                      \
      break;                                            \
    }                                                   \
  }                                                     \
}                                                       \


#define hsh_for(k,v,hsh)                            \
  for (int i_ = 0, j_; i_ <= (hsh)->size; i_++)     \
    if ((hsh)->keys[i_] && ((j_=1)))                \
      for (k=(hsh)->keys[i_];  j_; )                \
        for (v=(hsh)->elts[i_]; j_; j_=0)


#define hsh_ifor(k,v,hsh)                           \
  for (int i_ = 0, j_; i_ <= (hsh)->size; i_++)     \
    if (hsh_valid_int((hsh)->keys[i_]) && ((j_=1))) \
      for (k=(hsh)->keys[i_];  j_; )                \
        for (v=(hsh)->elts[i_]; j_; j_=0)


#if 0
#include "../include/hsh.h"

static char *months[] = {
   "jan","feb","mar","apr"
  ,"may","jun","jul","aug"
  ,"sep","oct","nov","dec"
  ,0
};

hsh_dcl(ht,cstr,char*,int)

void hsh_test() {
  ht h = HSH_INIT;

  printf("______POPULATING_____\n");
  for (int i = 0; months[i]; i++) {
    printf("adding %s = %d\n", months[i], i+1);
    *htAdd(&h,months[i],0) = i+1;
  }

  htErase(&h,"jul");
  htErase(&h,"may");
  htErase(&h,"sep");

  printf("______RETRIEVED______\n");
  for (int i = 0; i < 12; i++) {
    int *r = htGet(&h,months[i]);
    if (r) printf("%s = %d\n", months[i], *r);
  }
  printf("________TABLE________\n");
  printf("size=%d/%d\n", h.used, h.size);

  hsh_for(char*k, int v, &h) {
    printf("%s = %d\n", k, v);
  }
}
#endif

#endif
