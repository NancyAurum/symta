#include "util.h"

void memcpy4(U32 *RESTRICT p, U32*RESTRICT s, size_t len) {
  U32 *end = p + (len&~0x1f);
  while (p != end) {
    p[ 0] = s[0];
    p[ 1] = s[1];
    p[ 2] = s[2];
    p[ 3] = s[3];
    p[ 4] = s[4];
    p[ 5] = s[5];
    p[ 6] = s[6];
    p[ 7] = s[7];
    p[ 8] = s[8];
    p[ 9] = s[9];
    p[10] = s[10];
    p[11] = s[11];
    p[12] = s[12];
    p[13] = s[13];
    p[14] = s[14];
    p[15] = s[15];
    p[16] = s[16];
    p[17] = s[17];
    p[18] = s[18];
    p[19] = s[19];
    p[20] = s[20];
    p[21] = s[21];
    p[22] = s[22];
    p[23] = s[23];
    p[24] = s[24];
    p[25] = s[25];
    p[26] = s[26];
    p[27] = s[27];
    p[28] = s[28];
    p[29] = s[29];
    p[30] = s[30];
    p[31] = s[31];
    p += 32;
    s += 32;
  }
  end += len&0x1f;
  while (p != end) *p++ = *s++;
}

void memset4(U32 *RESTRICT p, U32 val, size_t len) {
  U32 *end = p + (len&~0x1f);
  while (p != end) {
    p[ 0] = val;
    p[ 1] = val;
    p[ 2] = val;
    p[ 3] = val;
    p[ 4] = val;
    p[ 5] = val;
    p[ 6] = val;
    p[ 7] = val;
    p[ 8] = val;
    p[ 9] = val;
    p[10] = val;
    p[11] = val;
    p[12] = val;
    p[13] = val;
    p[14] = val;
    p[15] = val;
    p[16] = val;
    p[17] = val;
    p[18] = val;
    p[19] = val;
    p[20] = val;
    p[21] = val;
    p[22] = val;
    p[23] = val;
    p[24] = val;
    p[25] = val;
    p[26] = val;
    p[27] = val;
    p[28] = val;
    p[29] = val;
    p[30] = val;
    p[31] = val;
    p += 32;
  }
  end += len&0x1f;
  while (p != end) *p++ = val;
}
