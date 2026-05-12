// Ken Silverman's voxel format's importers
#:slb
#:mesh

typedef struct {
  U32 col;
  U16 z;
  S8 vis;
  S8 dir;
} kv6voxtype;

TLst(kv6vs,kv6voxtype)

slab_t *vxImportKV6(char *filename) {
  int i;
  slab_t *slb = 0;
  U32 w,h,d;

  CFile in;

  if (0) {
fail:
    return 0;
  }

  if (!in.ropen(filename)) goto fail;
  if (in.readU32 != 0x6c78764b) goto fail;
  S32 xsiz = in.readS32;
  S32 ysiz = in.readS32;
  S32 zsiz = in.readS32;
  F32 xpiv = in.readF32;
  F32 ypiv = in.readF32;
  F32 zpiv = in.readF32;
  S32 numvoxs = in.readS32;
  //printf("WxHxD: %dx%dx%d\n", xsiz,ysiz,zsiz);
  //printf("pivot: %f,%f,%f\n", xpiv,ypiv,zpiv);
  //printf("numvoxs: %d\n", numvoxs);

  kv6voxtype *voxels = malloc(numvoxs*sizeof(kv6voxtype));

  for(i=0; i < numvoxs; i++) { //8 bytes per surface voxel, Z's must be sorted
    kv6voxtype &v = &voxels[i];
    U8 b = in.readU8;
    U8 g = in.readU8;
    U8 r = in.readU8;
    U8 a = in.readU8;
    v.col = RGB(r,g,b);
    v.z = in.readU16;   //Z coordinate of this surface voxel
    v.vis = in.readU8;  //Low 6 bits say if neighbor is solid or air
    v.dir = in.readU8;  //Uses 256-entry lookup table
  }

  U32s xlen = in.readU32s(xsiz);
  U16s ylen = in.readU16s(xsiz*ysiz);

  w = xsiz;
  h = zsiz;
  d = ysiz;

  slb = vxNew(w,h,d);

  U32 clay_color = 0xFF00FF;

  kv6voxtype *e = voxels + numvoxs;
  int pos = 0;
  for (int x = 0; x < xsiz; x++) {
    for (int y = 0; y < ysiz; y++) {
      int count = (int)ylen[x * ysiz + y];
      int last_z = -1;
      while (count--) {
        kv6voxtype &v = &voxels[pos];
        int z = v.z;
        attr_t *attr = slb.svo.set(x,zsiz-z-1,ysiz-y-1);
        attr->color = v.col;
        attr->normal = CLAY_NORMAL;
        pos++;
      }
    }
  }

  free(voxels);

  vxCompact(slb);

  vxFillInterior(slb);

  return slb;
}


slab_t *vxImportKVX(char *filename) {
  int i;
  slab_t *slb = 0;
  U32 w,h,d;

  CFile in;

  if (!in.ropen(filename)) return 0;

  S32 nbytes = in.readS32; //file_size - 4 - 256*3
  S32 fsize = in.size;
  if (fsize < 768+4 || fsize > (nbytes+4+256*3)*2)
    return 0; //file is two times larger than expected
              //likely not a Build engine KVX.
  S32 xsiz = in.readS32;
  S32 ysiz = in.readS32;
  S32 zsiz = in.readS32;
  //NOTE: original slab6 limits KVX files to 256x256x256
  //      and Build engine has even stricter limitations
  if (xsiz > 1024 || ysiz > 1024 || zsiz > 1024 || !xsiz || !ysiz || !zsiz){
    return 0;
  }
  //printf("nbytes: %d/%d\n", nbytes,fsize);
  //printf("WxHxD: %dx%dx%d\n", xsiz,ysiz,zsiz);

  F32 xpiv = (double)in.readS32/256.0;
  F32 ypiv = (double)in.readS32/256.0;
  F32 zpiv = (double)in.readS32/256.0;

  //printf("pivot: %f,%f,%f\n", xpiv,ypiv,zpiv);

  S32s xstart = in.readS32s(xsiz+1);
  U16s xyoffs = in.readU16s((ysiz+1)*xsiz);

  U64 datpos = in.tell;
  in.seek(in.size-768);
  U8s pal = in.readU8s(768);
  in.seek(datpos);
  
  for (int i = 0; i < 768; i++) {
    pal[i] = pal[i]<<2;
  }

  w = xsiz;
  h = zsiz;
  d = ysiz;

  slb = vxNew(w,h,d);

  U32 clay_color = 0xFF00FF;

  int x,y,z;
	for (x=0; x < xsiz; x++) {
		for (y=0; y < ysiz; y++) {
			i = xyoffs[x*(ysiz+1)+y+1] - xyoffs[x*(ysiz+1) + y];
      if (!i) continue;
			do {
				S32 z1 = in.readU8; //Starting z coordinate of top of slab
        S32 k = in.readU8; //# of bytes in the color array - slab height
        S32 cull = in.readU8; //Low 6 bits tell which of 6 faces are exposed
        i -= k+3;
        S32 z2 = z1+k;
				for(z = z1; z < z2; z++) {
          U32 ci = in.readU8;
          U32 color = RGB(pal[ci*3+0],pal[ci*3+1],pal[ci*3+2]);
          attr_t *attr = slb.svo.set(x,zsiz-z-1,ysiz-y-1);
          attr->color = color;
          attr->normal = CLAY_NORMAL;
				}
			} while (i);
		}
	}

  vxCompact(slb);
  vxFillInterior(slb);

  return slb;
}
