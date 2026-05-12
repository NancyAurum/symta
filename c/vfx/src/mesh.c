#:mesh


CMeshTexture *vxMeshNewTexture(int width, int height) {
  CMeshTexture *tx = new(CMeshTexture);
  tx->w = width;
  tx->h = height;
  tx->data = malloc(sizeof(U32)*width*height);
  memset(tx->data, 0, sizeof(U32)*width*height);
  return tx;
}


void CMeshTexture.clear(U32 color) {
  int i;
  int width = w;
  int height = h;
  for (i = 0; i < width*height; i++) {
    data[i] = color;
  }
}


void vxMeshFree(CMesh *mesh) {
  delete(mesh);
}

int CMeshTexture.write_tga(char *filename) {
  FILE *fptr = fopen(filename, "wb");
  if (!fptr) return 0;
  
  int width = w;
  int height = h;

  putc(0,fptr);                         /* length of id field */
  putc(0,fptr);                         /* color map type */
  putc(2,fptr);                         /* uncompressed RGB */
  putc(0,fptr); putc(0,fptr);           /* Color Map Origin */
  putc(0,fptr); putc(0,fptr);           /* Color Map Length. */
  putc(0,fptr);                         /* Color Map Entry Bit-Size. */
  putc(0,fptr); putc(0,fptr);           /* X origin */
  putc(0,fptr); putc(0,fptr);           /* Y origin */
  putc((width & 0x00FF),fptr);
  putc((width & 0xFF00) / 256,fptr);
  putc((height & 0x00FF),fptr);
  putc((height & 0xFF00) / 256,fptr);
  //putc(24,fptr);                        /* 24 bit bitmap */
  putc(32,fptr);                        /* 32 bit bitmap */
  putc(0,fptr);                         /* Image Descriptor Byte. */

  int x, y;
  for (y = height-1; y >= 0; y--) {
    for (x = 0; x < width; x++) {
      uint32_t color = data[y*width + x];
      int r,g,b,a;
      UNRGBA(r,g,b,a,color);
      putc(b,fptr);
      putc(g,fptr);
      putc(r,fptr);
      putc(0xFF,fptr);
    }
  }

  fclose(fptr);

  return 1;
}

void CMesh.rescale(F64 unit_scale, U32 flags) {
  auto vertices = &this->vertices;
  auto faces = &this->faces;

  F64 minX=INFINITY,minY=INFINITY,minZ=INFINITY;
  F64 maxX=-INFINITY,maxY=-INFINITY,maxZ=-INFINITY;
  for (int i = 0; i < vertices.len; i++) {
    CVertex *v = &vertices.elts[i];
    minX = fmin(minX,v->xyz.x);
    minY = fmin(minY,v->xyz.y);
    minZ = fmin(minZ,v->xyz.z);
    maxX = fmax(maxX,v->xyz.x);
    maxY = fmax(maxY,v->xyz.y);
    maxZ = fmax(maxZ,v->xyz.z);
  }

  //scale model into [0:1]
  F64 model_w = fmax(EPS,maxX-minX);
  F64 model_h = fmax(EPS,maxY-minY);
  F64 model_d = fmax(EPS,maxZ-minZ);
  F64 model_scale = fmax(model_d,fmax(model_w,model_h))*unit_scale;
  
  scale *= model_scale;
  model_scale = 1.0/model_scale;
  for (int i = 0; i < vertices.len; i++) {
    CVertex *v = &vertices.elts[i];
    F64 dx = (v->xyz.x-minX)*model_scale;
    if (flags&MESH_FLIP_X) dx = unit_scale-dx;
    F64 dy = (v->xyz.y-minY)*model_scale;
    if (flags&MESH_FLIP_Y) dy = unit_scale-dy;
    F64 dz = (v->xyz.z-minZ)*model_scale;
    if (flags&MESH_FLIP_Z) dz = unit_scale-dz;
    v->xyz.x = dx;
    v->xyz.y = dy;
    v->xyz.z = dz;
  }
}


int vxMeshSaveObj(char *filename, CMesh *mesh) {
  int i;
  //filename = "/Users/macbook/Downloads/export.obj";
  char *mtlname = 0;
  char *txdname = 0;
  url_t *url = 0;
  FILE *fptr = fopen(filename, "wb");
  if (!fptr) return 0;

  url = filename.url_parts;
  mtlname = cat(url->dir,"/",url->name, ".mtl");
  txdname = cat(url->dir,"/",url->name, ".tga");

  auto vs = mesh->vertices.elts;
  auto fs = mesh->faces.elts;

  fprintf(fptr,"# Made with VoxPie\n"); //comment

	if (mesh->flags&MESH_TEXTURED) {
    fprintf(fptr,"mtllib %s.mtl\n", url->name);
  }

  //fprintf(fptr,"g voxpie_export\n"); //group
  fprintf(fptr,"o voxpie_export\n"); //object name
  //fprintf(fptr,"\n");

	for (i = 0 ; i < mesh->vertices.len; i++) {
    float x = vs[i].xyz.x*2.0-1.0;
    float y = vs[i].xyz.y*2.0-1.0;
    float z = vs[i].xyz.z*2.0-1.0;
    fprintf(fptr,"v %g %g %g\n",x,y,z);
	}
  //fprintf(fptr,"\n");

	if (mesh->flags&MESH_TEXTURED) {
    for (i = 0 ; i < mesh->vertices.len; i++) {
      fprintf(fptr,"vt %08g %08g\n",vs[i].uv.x,vs[i].uv.y);
  	}
    //fprintf(fptr,"\n");
    //these `s` and `usemtl` can be change on per face basis
    fprintf(fptr,"usemtl Material.001\n");
    fprintf(fptr,"s 1\n");
  }

	for (i = 0 ; i < mesh->faces.len; i++) {
    int v0 = fs[i].vs[0]+1;
    int v1 = fs[i].vs[1]+1;
    int v2 = fs[i].vs[2]+1;
    if (mesh->flags&MESH_TEXTURED) {
      fprintf(fptr,"f %d/%d %d/%d %d/%d\n",v0,v0,v1,v1,v2,v2);
    } else {
      fprintf(fptr,"f %d %d %d\n",v0,v1,v2);
    }
	}
  //fprintf(fptr,"\n");

  fclose(fptr);

	if (mesh->flags&MESH_TEXTURED) {
    fptr = fopen(mtlname, "wb");
    fprintf(fptr,"newmtl Material.001\n");
    fprintf(fptr,"Ns 0.000000\n");
    fprintf(fptr,"Ka 1.000000 1.000000 1.000000\n");
    fprintf(fptr,"Kd 0.800000 0.800000 0.800000\n");
    fprintf(fptr,"Ks 0.000000 0.000000 0.000000\n");
    fprintf(fptr,"Ke 0.0 0.0 0.0\n");
    fprintf(fptr,"Ni 1.450000\n");
    fprintf(fptr,"d 1.000000\n");
    fprintf(fptr,"illum 1\n");
    //diffuse map
    fprintf(fptr,"map_Kd %s.tga\n", url->name);
    fclose(fptr);
    mesh->colors.write_tga(txdname);
  }

  if (mtlname) mFree(mtlname);
  if (txdname) mFree(txdname);
  if (url) free(url);
  return 1;
}

int vxExportObj(char *filename, slab_t *slb, int algorithm) {
  CMesh *mesh;
  if (algorithm == 0) {
    mesh = vxPolygonizeExact(slb);
  } else {
    mesh = vxPolygonize(slb);
  }
  int r = vxMeshSaveObj(filename, mesh);
  delete(mesh);
  return r;
}
