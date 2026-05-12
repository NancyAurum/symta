//Westwood's VXL format importer
#:slb
#:mesh
#:imp_vxl


/*

VXL file layout:
vxl_header  //file id, and part count
part declarations  //name and index for each part
part bodies        //RLE packed voxels for each part
part headers       //dimensions and coords for each for part

*/

typedef struct {
  U8 type[16];   //"Voxel Animation"
  U32 u01;       //always 1
  U32 nparts;    //number of part in this file
  U32 nparts2;   //same as nparts
  U32 bodysz;    //the size of all bodies combined
  U16 u02;       //0x1f10
  U8 pal[3*256]; //palette
} PACKED vxl_header_t;

typedef struct {
  U8 name[16];
  U32 index;
  U32 u01; //always 1
  U32 u02; //always 0
} PACKED vxl_part_decl_t;


typedef struct {
  U32 span_start_off;   // Offset into body section to span start list
  U32 span_end_off;     // Offset into body section to span end list
  U32 span_data_off;    // Offset into body section to span data
  F32 scale;
  F32 matrix[4*3];      //transformation matrix, has the form of
                        //   1 0 0 x
                        //   0 1 0 y
                        //   0 0 1 z
                        //and is used only to define the pivot (part center)
  F32 aabb_min[3];      //bounds
  F32 aabb_max[3];
  U8  xsize;            // Width of this part
  U8  ysize;            // Breadth of this part
  U8  zsize;            // Height of this part
  U8  normt;            // Normal table (TS uses 2, RA2 uses 4)
} PACKED vxl_part_head_t;

typedef struct {
  vxl_part_decl_t decl; //declarations
  vxl_part_head_t h; //header
} PACKED part_t;

TLst(parts_t, part_t)

//FIXME: VXL can contain multipart voxels
slab_t *vxImportVXL(char *filename) {
  int i;
  slab_t *slb = 0;
  U32 w,h,d;

  CFile in;
  vxl_header_t head;
  U8s body;

  if (!in.ropen(filename)) return 0;

  in.read(&head,sizeof(head));
  if (strncmp((char*)head.type,"Voxel Animation",16)) return 0;

  //printf("nparts: %d\n", head.nparts);

  parts_t parts;
  parts.init(head.nparts);

  //read part headers
  for (i = 0; i < head.nparts; i++) {
    part_t &part = &parts[i];
    in.read(&part.decl,sizeof(part.decl));
#if 0
    printf("part(%d): %s\n", part.decl.index, part.decl.name);
#endif
    parts.push(part);
	}

  body.init(head.bodysz);
  in.read(body.elts, head.bodysz);

  //in.seek(in.size - head.nparts*sizeof(vxl_part_head_t));

  for (i = 0; i < head.nparts; i++) {
    part_t &part = &parts[i];
    auto &t = part.h;
    in.read(&part.h,sizeof(part.h));
#if 0
    printf("Offset: %d\n", part.h.span_start_off);
    printf("Scale: %f\n", part.h.scale);
    printf("AABB Min: %f,%f,%f\n",t.aabb_min[0], t.aabb_min[1], t.aabb_min[2]);
    printf("AABB Max: %f,%f,%f\n",t.aabb_max[0], t.aabb_max[1], t.aabb_max[2]);
    printf("WxHxD: %dx%dx%d\n", t.xsize, t.ysize, t.zsize);
#endif
  }

  for (i = 0; i < 1; i++) {
    part_t &part = &parts[i];
    auto &t = part.h;
    
    F32 *nt;
    
    switch (t.normt) {
    case 1: nt = normals1; break;
    case 2: nt = normals2; break;
    case 3: nt = normals3; break;
    case 4: nt = normals4; break;
    default: nt = normals4; break;
    }

    w = t.ysize;
    h = t.zsize;
    d = t.xsize;
    slb = vxNew(w,h,d);

    int nspans = t.xsize*t.ysize;
    S32 *starts = (S32*)&body[t.span_start_off];
    S32 *ends = starts+nspans;
    U8 *voxels = (U8*)(ends+nspans);
    int span = 0; //current span
    int x,y,z;
    for (y = 0; y < t.ysize; y++) {
      for (x = 0; x < t.xsize; x++) {
        S32 start = starts[span];
        if (start == -1) goto span_end;
        U8 *p = voxels+start;
        U8 *pend = voxels+ends[span];
        z = 0;
        for (; p < pend && z < t.zsize; ) {
          int nempty = *p++;
          int nvoxels = *p++;
          z += nempty;
          for (; nvoxels > 0 && p < pend  && z < t.zsize; nvoxels--, z++) {
            U32 ci = *p++;
            U32 ni = *p++; //normal index
            vec3 n = {nt[ni*3+1],-nt[ni*3+2],-nt[ni*3+0]};
            U32 color = RGB(head.pal[ci*3+0],head.pal[ci*3+1],head.pal[ci*3+2]);
            attr_t *attr = slb.svo.set(y, z, t.xsize-x-1);
            attr->color = color;
            attr->normal = vnormal2rgb(n);
          }
          p++; //should be the same as of nvoxels
        }
span_end:
        span++;
      }
    }
  }

  vxCompact(slb);
  vxFillInterior(slb);

  return slb;
}
