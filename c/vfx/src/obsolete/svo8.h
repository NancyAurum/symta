#ifndef OT8_H
#define OT8_H

#include "common.h"
#include "vec3.h"

//when mask is not valid, it is just void/empty
#define OTN_VOID 0x000
//when mask is not valid, it is a terminating "interior" color.
//without associated normal
#define OTN_TERM 0x1FF

//8x8x8 cube node requires eight 9-bit indices, plus a mask
__attribute__((packed)) typedef struct {
  uint8_t bs[10];
} otn_node_t;

INLINE uint64_t otn_node_t.lo {return *(uint64_t*) this->bs;}
INLINE uint16_t otn_node_t.hi {return *(uint16_t*)(this->bs+8);}

INLINE void otn_node_t."lo="(uint64_t v) {*(uint64_t*) this->bs    = v;}
INLINE void otn_node_t."hi="(uint16_t v) {*(uint16_t*)(this->bs+8) = v;}

INLINE uint32_t otn_node_t.oct(int i) {
  return i != 7 ? ((this.lo>>(i*9)) & 0x1FF)
                : ((this.hi>>8)<<1) | (this.lo>>63);
}

INLINE uint32_t otn_node_t.vmask {
  return this.hi&0xFF;
}

INLINE void otn_node_t."vmask="(uint8_t m) {
  this.hi = (this.hi&0xFF00) | m;
}

typedef struct {
  uint32_t color;
  uint32_t normal;
} otn_attr_t;

static otn_attr_t otn_term;

CXLst(otn_nodes_t,otn_node_t)
CXLst(otn_attrs_t,otn_attr_t)


typedef struct ot_t {
  otn_nodes_t topo; //topology
  otn_attrs_t attr; //attributes
  void **fattr;     //free attrs list
  uint32_t c;       //cube center x,y,z
} otn_t;


INLINE otn_attr_t *otn_t.get_attr(uint32_t x, uint32_t y, uint32_t z) {
  uint32_t ptr = 0;
  uint32_t c = this->c;
  for(;;) {
    otn_node_t *n = this->topo.ref(ptr);
    uint32_t oct = 0;
    if (x >= c) {oct  = 0x1; x -= c;}
    if (y >= c) {oct |= 0x2; y -= c;}
    if (z >= c) {oct |= 0x4; z -= c;}
    ptr = n.oct(oct);
    if (!(n.vmask&oct)) return ptr == OTN_TERM ? &otn_term : 0;
    if (!c) return this->attr.ref(ptr);
    c>>=1;
  }
}

INLINE void otn_t.set(uint32_t x, uint32_t y, uint32_t z, uint32_t color) {
  uint32_t ptr = 0;
  uint32_t c = this->c;
  for(;;) {
    otn_node_t *n = this->topo.ref(ptr);
    uint32_t oct = 0;
    if (x >= c) {oct  = 0x1; x -= c;}
    if (y >= c) {oct |= 0x2; y -= c;}
    if (z >= c) {oct |= 0x4; z -= c;}
    ptr = n.oct(oct);
    if (!(n.vmask&oct)) {
      otn_attrs_t *attr;
      if (ptr == OTN_TERM) {
        if (!this->fattr) goto alloc_attr;
        attr = (otn_attrs_t*)this->fattr;
        this->fattr = (void**)*this->fattr;
      } else {
alloc_attr:
        n.set_oct(oct,this->attr.used);
        attr = this.attr.add;
      }
      attr->color = color;
      attr->normal = 0;
      return;
    }
    if (!c) {
      this->attr.ref(ptr)->color = color;
      return;
    }
    c>>=1;
  }
}

#endif
