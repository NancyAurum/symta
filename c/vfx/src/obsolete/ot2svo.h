#define OB(oct) (1<<(oct))


static
void ot2svo_r(svo_t *svo, uint32_t pdesc, 
               nodes_t *RESTRICT tree, uint32_t index) {
  int oct;

  uint32_t *octs = tree.ref(index)->octs;
  uint32_t leaf_mask = 0;
  uint32_t valid_mask = 0;
  for (oct = 0; oct < 8; oct++) {
    uint32_t voxel = octs[oct];
    if (OT_COLOR(voxel)) {
      uint32_t bit = OB(oct);
      valid_mask |= bit;
      if (voxel&OT_TERM) {
        leaf_mask |= bit;
      }
    }
  }
  uint32_t branch_mask = (leaf_mask^0xff)&valid_mask;
  
  //fprintf(stderr, "%x:%x:%x\n", pdesc, valid_mask, leaf_mask);

  // write descriptor
  svo->topo.elts[pdesc+0] = (valid_mask<<8) | branch_mask;
  
  uint32_t oattr = 0;
  
  if (leaf_mask) {
    oattr = svo->attr.used;
    for (oct = 0; oct < 8; oct++) {
      if (leaf_mask&OB(oct)) {
        attr_t value;
        value.color = OT_COLOR(octs[oct]);
        value.normal = 0;
        svo->attr.push(value);
      }
    }
  }

  // write branches
  if (branch_mask) {
    int nkids = popc8(branch_mask);
    uint32_t pkids = svo->topo.used;
    //fprintf(stderr, "used=%d/%d\n", svo->topo.used, svo->topo.size);
    svo->topo.elts[pdesc+1] = pkids;

    for (oct = 0; oct < 8; oct++) {
      if (branch_mask&OB(oct)) {
        svo->topo.push(0);
        svo->topo.push(0);
      }
    }
    svo->topo.push(oattr);

    for (oct = 0; oct < 8; oct++) {
      if (branch_mask&OB(oct)) {
        //fprintf(stderr, "%d:childS: %d\n", node,oct);
        ot2svo_r(svo, pkids, tree, octs[oct]);
        pkids += 2;
      }
    }
  } else {
    svo->topo.elts[pdesc+1] = oattr;
  }

  /*fprintf(stderr, "%d:build: desc=%x, nkids=%x\n"
                , node, out->base[pdesc+0], nkids);*/

}

#undef OB

static void ot2svo(svo_t *svo, nodes_t *RESTRICT tree) {
  int c = svo->c;
  svo_free(svo);
  svo_init(svo, 0);
  svo->c = c;
  ot2svo_r(svo, 0, tree, 0);
}
