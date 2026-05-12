INLINE void memset4(uint32_t *RESTRICT p, uint32_t val, int len) {
  uint32_t *end = p + (len&~0x1f);
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
