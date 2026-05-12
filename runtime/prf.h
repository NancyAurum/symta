//Adaptive Map
#ifndef PRF_H
#define PRF_H

#include "ng.h"


#ifdef PRF_ACTIVE
#define PRF(v) prf.v++;
#else
#define PRF(v)
#endif

typedef struct {
  uint64_t gid_get_, gid_set_;
} prf_t;


#ifdef PRF_IMPLEMENTATION
prf_t prf;
#undef PRF_IMPLEMENTATION
#else
extern prf_t prf;
#endif


#endif
