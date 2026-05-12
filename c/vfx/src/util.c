#:slb

void generate_normal_sum_loop() {
  int i;
  int x,y,z;
  for (i = 0; i < 1; i++) {
    int c = i+1;
    int s = c+c+1;
    int n = s*s*s-1;
    //printf("%d\n", n); continue;
    printf("vec3 calc_normal%d(slab_t *slb, ivec3 p) {\n", i);
    printf("ivec3 dir;\n");
    printf("ivec3 q = p-(ivec3){1,1,1};\n");
    printf("ivec3 r = p+(ivec3){1,1,1};\n");
    printf("int w = slb->w, h = slb->h, d = slb->d;\n");
    printf("ivec3 n = {0,0,0};\n");
    printf("if ((U32)q.x<w&&(U32)q.y<h&&(U32)q.z<d&&\n");
    printf("    (U32)r.x<w&&(U32)r.y<h&&(U32)r.z<d) {\n");
    for (z = -c; z <= c; z++) { //cast rays into all directions
      for (y = -c; y <= c; y++) {
        for (x = -c; x <= c; x++) {
          if (x == 0 && y == 0 && z == 0) continue;
          printf("dir = (ivec3){%d,%d,%d};\n", x,y,z);
          printf("q = p+dir;\n");
          printf("if (slb.svo.nil(q.x,q.y,q.z)) n += dir;\n");
        }
      }
    }
    printf("} else {\n");
    for (z = -c; z <= c; z++) { //cast rays into all directions
      for (y = -c; y <= c; y++) {
        for (x = -c; x <= c; x++) {
          if (x == 0 && y == 0 && z == 0) continue;
          printf("dir = (ivec3){%d,%d,%d};\n", x,y,z);
          printf("q = p+dir;\n");
          printf("if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d\n");
          printf("   || slb.svo.nil(q.x,q.y,q.z)) {\n");
          printf("  n += dir;\n");
          printf("}\n");
        }
      }
    }
    printf("}\n");
    printf("return normalized((vec3){n.x,n.y,n.z});\n");
    printf("}\n");
  }
}

void generate_normal_sum_loop2() {
  for (int soft = 0; soft < 16; soft++) {
    int c = soft+1;
    printf("case %d:\n", c);
    for (int x = -c; x <= c; x++) {
      printf("  if (seg.nil16(xxx,yyy,zzz)) n += (ivec3){%d,y,z};\n", x);
      if (x != c) printf("  xxx++;\n");
    }
    printf("break;\n");
  }
}

void generate_seg_terms() {
  printf("static U32 oct_vecs[] = {\n");
  for (int b = 4; b >= 0; b--) {
    int c = 1<<b;
    printf("  ");
    for (int i = 0; i < 8; i++) {
      U32 ox = c*( i    &1);
      U32 oy = c*((i>>1)&1);
      U32 oz = c* (i>>2)   ;
      printf("0x%02X", ox|(oy<<10)|(oz<<20));
      if (i != 7) printf(",");
      //if (i == 3) printf("\n  ");
    }
    if (b != 0) printf(",");
    printf("\n");
  }
  printf("};\n");
}

int main(int argc, char **argv) {
  //generate_normal_sum_loop();
  generate_seg_terms();
  return 0;
}
