static void memcpy4(uint32_t *restrict p, uint32_t*restrict s, int len) {
  uint32_t *end = p + (len&~0x1f);
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
