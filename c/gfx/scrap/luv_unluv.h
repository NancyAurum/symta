

#define LUV(l,u,v) (((l)<<16)|((u)<<8)|(v))
#define UNLUV(L,U,V,C) do { \
  uint32_t _fromC = (C)&0xFFFFFFFF; \
  V = ((_fromC)&0xFF); \
  U = (((_fromC)>> 8)&0xFF); \
  L = ((_fromC)>>16); \
 } while (0)
