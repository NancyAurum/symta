#ifndef NC_UTIL_H
#define NC_UTIL_H

#include "type.h"
#include "defs.h"

void memcpy4(U32 *RESTRICT p, U32*RESTRICT s, size_t len);
void memset4(U32 *RESTRICT p, U32 val, size_t len);

#endif
