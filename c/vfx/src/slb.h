#:common
#:svo

//slb->flags
#SLB_LIGHT      0x01
#SLB_INVISIBLE  0x02

struct slab_t { //voxel slab
  ot_t svo;
  U32 w,h,d;        //width,height,depth
  U32 flags;        //flags describing the slab
  vec3 box;         //AABB for this slab (model space)
  vec3 scale;       //scale of the slab along x,y,z axes
  vec3 center;      //center of rotation
  vec3 angle;       //rotation around center
  vec3 xyz;         //location in world
  vec3 color;       //color for lights
  char *info;       //text info set by user
  U32 nil;          //getTr returns this for the empty voxels
  U32 abyss;        //getTr returns this for the abyss
};

#XYZ_OUTSIDE(x,y,z,w,h,d) {
   (   (U32)(x) >= (U32)(w)
    || (U32)(y) >= (U32)(h)
    || (U32)(z) >= (U32)(d))
}

//check is x,y,z are on the slab border or outside of it
//uses the fact that 0-1 = 0xFFFFFFFF
#XYZ_BORDER_OR_OUTSIDE(x,y,z,w,h,d) {
   (   (U32)(x)-1 >= (U32)(w)-2
    || (U32)(y)-1 >= (U32)(h)-2
    || (U32)(z)-1 >= (U32)(d)-2)
}

inline ot_t *slab_t.svo {return &this->svo;}
inline U32 slab_t.c {return this->svo.c;}

inline int slab_t.contains(int x, int y, int z) {
  return (U32)x < this->w || (U32)y < this->h || (U32)z < this->d;
}

inline int slab_t.vcontains(ivec3 p) {
  return (U32)p.x < this->w || (U32)p.y < this->h || (U32)p.z < this->d;
}

inline U32 slab_t.get_color(U32 x, U32 y, U32 z) {
  if (XYZ_OUTSIDE(x,y,z,this->w,this->h,this->d)) return NIL_COLOR;
  attr_t *attr = this.svo.get(x, y, z);
  if (attr.is_empty) return NIL_COLOR;
  return attr->color;
}

typedef struct {
  vec3 min;
  vec3 max;
} aabb_t;

//we must decouple model view from slab_t, since we want different instancing
//for the same slb
typedef struct {
  slab_t *slb;
  mat3 view;      //model view rotation and scale
  vec3 o;         //offset of the slab inside the scene (model view coords)
  vec3 scale;     //scale applied to model
  aabb_t aabb;    //AABB in the world space
  int index;      //our index on the scene items list
  mat3 rview;     //reverse model view rotation and scale
} scene_item_t;

scene_item_t slab_to_scene_item(slab_t *slb);

attr_t *vxRecalcNormal(slab_t *slb, vec3 p, vec3 *rn, U32 soft);

extern camera_t gcam;

extern char slb_sample_txt[1024];

extern U32 gsoftness;

slab_t *vxNew(U32 w, U32 h, U32 d);
void vxFree(slab_t *slb);
void vxClear(slab_t *slb, U32 color);
void vxOrient(slab_t *slb, U32 flags
               ,float sx, float sy, float sz
               ,float cx, float cy, float cz
               ,float ax, float ay, float az
               ,float x, float y, float z
               ,float r, float g, float b);

char *vxInfo(slab_t *slb);
void vxSetInfo(slab_t *slb, char *info);
void vxCompact(slab_t *slb);
U32 vxGetClayColor(slab_t *slb);
void vxSetClayColor(slab_t *slb, U32 color);
void vxCutOutlierTerms(slab_t *slb);
void vxFillInterior(slab_t *slb);
int vxImportPly(slab_t *slb, char *filename);

slab_t *vxImportKV6(char *filename);
slab_t *vxImportKVX(char *filename);
slab_t *vxImportVXL(char *filename);

inline void rt_t.shot {
  this->svo.raycast(this);
}

inline void rt_t.init(slab_t *slb) {
  this->svo = slb.svo;
  this->box = slb->box;
}
