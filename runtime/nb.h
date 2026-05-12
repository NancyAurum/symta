//Nancy's Bitmap

#ifndef NB_H
#define NB_H

//#define NB_SAVE_MEMORY
#define NB_ID uint64_t
#define NB_WORD uint64_t

//GCC generates worse code for uint128_t than for uint64_t[2]
//#define NB_WORD __uint128_t

//64bytes = size of x86 cache line
#define NB_NWORDS 8

//NOTE: we have a few bits left free in the hash map's key

#include "mlog2.h"

#define NB_PBITS (sizeof(NB_WORD)*8*NB_NWORDS)
#define NB_PSHFT mlog2(NB_PBITS-1)
#define NB_PMASK ((1<<NB_PSHFT)-1)


#define NB_WBITS (sizeof(NB_WORD)*8)
#define NB_WSHFT mlog2(NB_WBITS-1)
#define NB_WMASK ((1<<NB_WSHFT)-1)

#define NB_WFULL (~(NB_WORD)0)

#define NB_ID_NIL (~(NB_ID)0)


#ifdef NB_SAVE_MEMORY
typedef struct { NB_WORD *words; } nb_page_t;
INLINE void nbInitVal_(nb_page_t *page) {
  page->words = malloc(sizeof(NB_WORD)*NB_NWORDS);
  for (int j = 0; j < NB_NWORDS; j++) page->words[j] = 0;
}
INLINE void nbFreeVal_(nb_page_t *page) {free(page->words);}
#else
typedef struct { NB_WORD words[NB_NWORDS]; } nb_page_t;
INLINE void nbInitVal_(nb_page_t *page) {
  for (int j = 0; j < NB_NWORDS; j++) page->words[j] = 0;
}
INLINE void nbFreeVal_(nb_page_t *page) {}
#endif


#define NH_KEY NB_ID
#define NH_VAL nb_page_t
#define NH_KEY_NIL NB_ID_NIL

#define NH_INIT_VAL(x) nbInitVal_(&x)

#define NH_PREP
#define NH_PFX nhPg

#include "nh.h"


typedef nhPg_t *nb_t;

INLINE nb_t nbAlloc() {
  return nhPgAlloc();
}

INLINE void nbFree(nb_t nb) {
  return nhPgFree(nb);
}

INLINE NB_WORD nbGet(nb_t nb, NB_ID id) {
  NB_ID key = id >> NB_PSHFT;
  nb_page_t *page = nhPgLookup(nb,key);
  if (!page) return 0;
  NB_ID bsh = id & NB_WMASK;
  NB_WORD bit = (NB_WORD)1<<bsh;
  NB_ID wid = (id&NB_PMASK)>>NB_WSHFT;
  NB_WORD word = page->words[wid];
  return word & bit;
}

INLINE void nbSet(nb_t *nb, NB_ID id) {
  nhPgAdd(nb, id>>NB_PSHFT, 0)->words[(id&NB_PMASK)>>NB_WSHFT]
    |= (NB_WORD)1 << (id & NB_WMASK);
}

INLINE void nbDel(nb_t *nb, NB_ID id) {
  NB_ID key = id >> NB_PSHFT;
  nb_page_t *page = nhPgLookup(*nb,key);
  if (!page) return;
  NB_ID bsh = id & NB_WMASK;
  NB_WORD bit = (NB_WORD)1<<bsh;
  NB_ID wid = (id&NB_PMASK)>>NB_WSHFT;
  NB_WORD *pwords = page->words;
  pwords[wid] &= ~bit;
  for (int j = 0; j < NB_NWORDS; j++) if (pwords[j]) return;
  nbFreeVal_(page);
  nhPgDel(nb, key);
}



INLINE NB_ID nbN(nb_t nb) { //population count
  NB_ID used = 0;
  times(i, nhPgCap(nb)) {
    if (nb->keys[i] == NB_ID_NIL) continue;
    NB_WORD *pwords = nb->vals[i].words;
    for (int j = 0; j < NB_NWORDS; j++) {
      NB_WORD word = pwords[j];
      times(k, NB_WBITS) {
        if (word & ((NB_WORD)1<<k)) ++used;
      }
    }
  }
  return used;
}

INLINE NB_ID nbSize(nb_t nb) {
  NB_ID size = 0;
  times(i, nhPgCap(nb)) {
    if (nb->keys[i] == NB_ID_NIL) continue;
    size += NB_PBITS;
  }
  return size;
}

INLINE void nbInfo(nb_t nb) {
  printf("npages=%d\n", nhPgN(nb));
  times(i, nhPgCap(nb)) {
    if (nb->keys[i] == NB_ID_NIL) continue;
    NB_ID key = nb->keys[i];
    NB_WORD *pwords = nb->vals[i].words;
    printf("page #%d\n", key);
    for (int j = 0; j < NB_NWORDS; j++) {
      NB_WORD word = pwords[j];
      //printf("word #%d = %016llx\n", j, word);
      times(k, NB_WBITS) {
        if (word & ((NB_WORD)1<<k)) {
          NB_ID id = (key<<NB_PSHFT) | (j << NB_WSHFT) | k;
          printf("  %lld\n", id);
        }
      }
    }
  }
}

#define NB_FOR(id,nb) \
  NB_WORD word; times(i_, nhPgCap(nb)) \
    LET(NB_ID key_ = nb->keys[i_]) \
      if (key_ != NB_ID_NIL) \
        LET(NB_WORD* pwords = nb->vals[i_].words) times(j_, NB_NWORDS) \
         LET(NB_WORD word = pwords[j_]) times(k_, NB_WBITS) \
           if (word & ((NB_WORD)1<<k_)) \
             LET(NB_ID id = (key_<<NB_PSHFT) | (j_ << NB_WSHFT) | k_)


#if 0

INLINE void nbTest() {
  nb_t nb = nbAlloc();
  nbSet(&nb,123);
  nbSet(&nb,127);
  nbSet(&nb,120000);
  nbSet(&nb,200);
  nbDel(&nb, 120000);
  //nbDel(&nb, 123);
  printf("used: %d/%d\n", nbN(nb), nbSize(nb));
  nbInfo(nb);

  //NB_FOR(id,nb) printf("%lld\n", id);

  exit(0);
}

INLINE void nbTest2() {
  nb_t nb = nbAlloc();
  for (NB_ID i = 0; i < (NB_ID)1<<32; i++) {
    if (i&1)
      nbSet(&nb,i);
  }

  printf("capacity: %lld\n", nhPgCap(nb));
  printf("npages: %lld\n", nhPgN(nb));
  printf("estimated size: %lld bytes\n", nhPgN(nb)*16);
  exit(0);
}


INLINE void nbTest3() {
  nb_t nb = nbAlloc();
  for (NB_ID i = 0; i < (NB_ID)1<<32; i++) {
    //nbSet(&nb,i);
    if (i&1) nbSet(&nb,i);
  }

  uint64_t count = 0;
  for (NB_ID j = 0; j < (NB_ID)10; j++) {
    for (NB_ID i = 0; i < (NB_ID)1<<32; i++) {
       if (nbGet(nb,i)) ++count;
    }
    printf("%lld\n", count);
  }


  exit(0);
}

#endif


INLINE void nbTest() {
  printf("hello\n");
  nb_t nb = nbAlloc();
  nbSet(&nb,123);
  nbSet(&nb,127);
  nbSet(&nb,120000);
  nbSet(&nb,200);
  //nbDel(&nb, 120000);
  //nbDel(&nb, 123);
  printf("used: %d/%d\n", nbN(nb), nbSize(nb));



  //nbInfo(nb);
  exit(0);
}


#endif
