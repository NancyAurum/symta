#:slb

class CVertex {
  vec3 xyz;
  vec2 uv;
  //vec3 normal;
}

class CFace {
  U32 vs[3]; //vertices
}

TLst(CVertexList,CVertex)
TLst(CFaceList,CFace)


class CMeshTexture {
  int w;
  int h;
  uint32_t *data; //ARGB color values
}

CTOR(CMeshTexture) {
  w = 0;
  h = 0;
  data = 0;
}

DTOR(CMeshTexture) {
  if (data) free(data);
}


class CMesh {
  CVertexList vertices;
  CFaceList faces;
  double scale;
  uint32_t flags;
  CMeshTexture *colors;
}

CTOR(CMesh) {
  flags = 0;
  scale = 1.0;
  colors = 0;
}

DTOR(CMesh) {
  if (colors) delete(colors);
}

CMeshTexture *vxMeshNewTexture(int width, int height);
void CMeshTexture.clear(U32 color);

#MESH_TEXELS_PER_LINE 1024

#MESH_FLIP_X      0x01
#MESH_FLIP_Y      0x02
#MESH_FLIP_Z      0x04
#MESH_POSITIVE    0x08

#MESH_FAN       0x01
#MESH_STRIP     0x02
#MESH_NORMALS   0x04
#MESH_TEXTURED  0x08


void vxVoxelize(slab_t *slb, CMesh *mesh);

void CMesh.rescale(F64 unit_scale, U32 flags);
int vxImportObj(slab_t *slb, char *filename);
int vxMeshSaveObj(char *filename, CMesh *mesh);

CMesh *vxPolygonize(slab_t *slb);
CMesh *vxPolygonizeExact(slab_t *slb);
