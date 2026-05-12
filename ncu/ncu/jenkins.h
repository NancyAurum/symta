#define jenkins_rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

#define jenkins_mix(a,b,c) \
{ \
  a -= c;  a ^= jenkins_rot(c, 4);  c += b; \
  b -= a;  b ^= jenkins_rot(a, 6);  a += c; \
  c -= b;  c ^= jenkins_rot(b, 8);  b += a; \
  a -= c;  a ^= jenkins_rot(c,16);  c += b; \
  b -= a;  b ^= jenkins_rot(a,19);  a += c; \
  c -= b;  c ^= jenkins_rot(b, 4);  b += a; \
}

#define jenkins_final(a,b,c) \
{ \
  c ^= b; c -= jenkins_rot(b,14); \
  a ^= c; a -= jenkins_rot(c,11); \
  b ^= a; b -= jenkins_rot(a,25); \
  c ^= b; c -= jenkins_rot(b,16); \
  a ^= c; a -= jenkins_rot(c,4);  \
  b ^= a; b -= jenkins_rot(a,14); \
  c ^= b; c -= jenkins_rot(b,24); \
}

static inline uint32_t
jenkins_lookup3(const void * key, int length, uint32_t initval ) {
  uint32_t a,b,c;  /* internal state */

  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  const uint32_t *k = (const uint32_t *)key;   /* read 32-bit chunks */

  /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
  while (length > 12) {
    a += k[0];
    b += k[1];
    c += k[2];
    jenkins_mix(a,b,c);
    length -= 12;
    k += 3;
  }

  switch(length)
  {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : { return c; }  /* zero length strings require no mixing */
  }

  jenkins_final(a,b,c);

  return c;
}

#if 0

//old jenkins hash one_at_a_time hash
uint32_t hsh_cstr(void *in, uint32_t sz) {
  uint8_t* key = (uint8_t*)in;
  size_t i = 0;
  uint32_t hash = 0;
  while (key[i]) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash&sz;
}

#endif