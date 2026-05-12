/* Voxel Graphics Library

TODO:
- Break this file into shader, editor, scene-builder and render components.

*/

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>

#include <gfx.h>

#include "dbg.h"

#include "profile.h"


#include "common.h"
#include "gfx_primitives.h"

@cext

class !gfx_t;

//base camera distance where culling happens
#define BASE_DISTANCE 1000.0
//#define BASE_DISTANCE 10.0

#if 0
#include "svo.h"
#else
#include "grid.h"
#endif

//for vxDbg
int dbg = 0;

static int error_halt(int n) {
  return 1/n;
}

typedef struct {
  vec3 focus;
  vec3 angle;
  float distance;
  float scale;
  U32 screen_w;
  U32 screen_h;
} camera_t;

static camera_t gcam;


//slb->flags
#define SLB_LIGHT      0x01
#define SLB_INVISIBLE  0x02
#define SLB_FOCUS      0x04

struct slab_t { //voxel slab
  SVO_T svo;
  U32 w,h,d;        //width,height,depth
  U32 flags;        //flags describing the slab
  vec3 box;         //AABB for this slab (model space)
  vec3 scale;       //scale of the slab along x,y,z axes
  vec3 center;      //center of rotation
  vec3 angle;       //rotation around center
  vec3 xyz;         //location in world
  vec3 color;       //color for lights
  SVOTerms *mesh;   //mesh voxel terms cache
  char *info;       //text info set by user
};

INLINE SVO_T *slab_t.svo {return &this->svo;}
INLINE U32 slab_t.c {return this->svo.c;}

INLINE void rt_t.shot { //FIXME
  Chunk *chunk = this->chunk;
  if (!chunk) return;
  chunk.raycast(this);
}

static SVOTerms *vxListTerms(slab_t *slb) {
  return slb.svo.terms;
}

static void vxPrintTerms(slab_t *slb) {
  auto terms = vxListTerms(slb);
  fprintf(stderr,"terms: %d/%d\n", terms->used, terms->size);
  int i;
  for (i = 0; i < terms->used; i++) {
    auto t = terms.ref(i);
    fprintf(stderr, "%d: %d,%d,%d,%c\n", i, t->o.x, t->o.y, t->o.z, t->c);
  }
  delete(terms);
}



//FIXME: implement special get_neibs() function,
//                 which can gather up to 4 voxels at once
static vec3 get_normal(slab_t *slb, vec3 p) {
#if 0
#define NNORM_DIRS 6
  vec3 ds[] = {
    { 1,0,0},{0, 1,0},{0,0, 1},
    {-1,0,0},{0,-1,0},{0,0,-1},
  };

#else
#define NNORM_DIRS 26
  vec3 ds[] = {
   {-1,-1,-1},{-1,-1, 0},{-1,-1, 1}
  ,{-1, 0,-1},{-1, 0, 0},{-1, 0, 1}
  ,{-1, 1,-1},{-1, 1, 0},{-1, 1, 1}

  ,{ 0,-1,-1},{ 0,-1, 0},{ 0,-1, 1}
  ,{ 0, 0,-1},/*{ 0, 0, 0},*/{ 0, 0, 1}
  ,{ 0, 1,-1},{ 0, 1, 0},{ 0, 1, 1}

  ,{ 1,-1,-1},{ 1,-1, 0},{ 1,-1, 1}
  ,{ 1, 0,-1},{ 1, 0, 0},{ 1, 0, 1}
  ,{ 1, 1,-1},{ 1, 1, 0},{ 1, 1, 1}
  };
#endif
  int i;
  vec3 n = {0,0,0};
  int count = 0;
  float w = slb->w;
  float h = slb->h;
  float d = slb->d;
  for (i = 0; i < NNORM_DIRS; i++) {
    vec3 q = p+ds[i];
    if (q.x<0.0f || q.y<0.0f || q.z<0.0f || q.x>=w || q.y>=h || q.z>=d) {
      goto add;
    }
    attr_t *attr = slb.svo.getv(q);
    if (!attr.is_empty) continue;
add:
    n += normalized(ds[i]);
    count++;
  }
  if (count) n /= (float)count;
  return n;
}

//calc normals for all the voxels on the box faces
static void vxRecalcNormalsBox(slab_t *slb
   ,int32_t sx, int32_t sy, int32_t sz
   ,int32_t w, int32_t h, int32_t d) {
  int32_t x,y,z;
  auto tree = slb.svo;
  int32_t ex=sx+w, ey=sy+h, ez=sz+d;

  if (sx < 0) sx = 0;
  if (sy < 0) sy = 0;
  if (sz < 0) sz = 0;
  /*if (ex >= slb->w) ex = slb->w-1;
  if (ey >= slb->h) ey = slb->h-1;
  if (ez >= slb->d) ez = slb->d-1;*/

  //FIXME: the following can be optimized, since the goal is only to split terms
  //       into fine grained mesh
#define REMESH_BODY \
  { \
    attr_t *v = tree.get(x,y,z); \
    if (v.is_empty) continue; \
    attr_t *attr = tree.set(x,y,z); \
    attr->normal = MESH_NORMAL; \
  }

  x = sx;
  //for (x = sx; x < ex; x++)
  for (y = sy; y < ey; y++)
  for (z = sz; z < ez; z++)
    REMESH_BODY

  y = sy;
  for (x = sx; x < ex; x++)
  //for (y = sy; y < ey; y++)
  for (z = sz; z < ez; z++)
    REMESH_BODY

  z = sz;
  for (x = sx; x < ex; x++)
  for (y = sy; y < ey; y++)
  //for (z = sz; z < ez; z++)
    REMESH_BODY

  x = ex-1;
  //for (x = sx; x < ex; x++)
  for (y = sy; y < ey; y++)
  for (z = sz; z < ez; z++)
    REMESH_BODY

  y = ey-1;
  for (x = sx; x < ex; x++)
  //for (y = sy; y < ey; y++)
  for (z = sz; z < ez; z++)
    REMESH_BODY

  z = ez-1;
  for (x = sx; x < ex; x++)
  for (y = sy; y < ey; y++)
  //for (z = sz; z < ez; z++)
    REMESH_BODY
}


void vxRecalcNormals(slab_t *slb) {
  auto tree = slb.svo;
  vxRecalcNormalsBox(slb,0,0,0,slb->w,slb->h,slb->d);
  SVOTerms *terms = vxListTerms(slb);
  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    if (tree.exists(t)) continue; //we care only about empty terms
    int32_t sx = t->o.x;
    int32_t sy = t->o.y;
    int32_t sz = t->o.z;
    int32_t cc = t->c+2;
    vxRecalcNormalsBox(slb,sx-1,sy-1,sz-1,cc,cc,cc);
  }
  delete(terms);
}


static void cut_box(slab_t *slb, int invert
                  , U32 bx, U32 by, U32 bz
                  , U32 bw, U32 bh, U32 bd);

void vxNormalizeBox(slab_t *slb) {
  vxRecalcNormals(slb);
  cut_box(slb, 1, 0,0,0,slb->w,slb->h,slb->d);
}


void vxOptimize(slab_t *slb) {
  slb.svo.optimize;
}

void vxRealloc(slab_t *slb) {
  vxOptimize(slb);
  slb.svo.realloc;
}

void vxCompact(slab_t *slb);

static SVOTerms *vxMeshTerms(slab_t *slb) {
  if (slb->mesh) return slb->mesh;
  vxRecalcNormals(slb);
  vxCompact(slb);
  slb->mesh = slb.svo.mesh_terms;
  return slb->mesh;
}

void vxRemesh(slab_t *slb, int now) {
  if (!slb->mesh) goto end;
  delete(slb->mesh);
  slb->mesh = 0;
end:
  if (now) vxMeshTerms(slb);
}

void vxCompact(slab_t *slb) {
  vxRemesh(slb, 0);
  vxOptimize(slb);
  slb.svo.realloc;
}

//clear interior of the mesh
void vxClearInterior(slab_t *slb, U32 clear_color) {
  vxRemesh(slb, 0);
  vxMeshTerms(slb);
  slb.svo.clear_interior;
  vxCompact(slb);
}



void vxSetClayColor(slab_t *slb, U32 color) {
  slb.svo.set_clay_color(color);
}

INLINE void vxClearNoSVO(slab_t *slb, U32 color) {
  vxRemesh(slb, 0);
  if (color) vxNormalizeBox(slb);
}

void vxClear(slab_t *slb, U32 color) {
  slb.svo.clear(color);
  vxClearNoSVO(slb,color);
}

static U32 whd2scale(U32 w, U32 h, U32 d) {
  U32 c = 1;
  U32 g = (MAX(MAX(w,h),d)+1)/2; //align to neares power of 2
  U32 scale = 0;
  while (c < g) {
    c *= 2;
    scale++;
  }
  return scale;
}

static void init_slb(slab_t *slb, U32 w, U32 h, U32 d) {
  slb->w = w;
  slb->h = h;
  slb->d = d;
  slb->box = (vec3){w+EPS,h+EPS,d+EPS};
  slb->flags = 0;
  slb->info = 0;
  U32 scale = whd2scale(w,h,d);
  slb.svo.init(scale);
}

slab_t *vxNew(U32 w, U32 h, U32 d) {
  slab_t *slb = (slab_t*)malloc(sizeof(slab_t));
  if (!slb) return 0;
  slb->mesh = 0;
  init_slb(slb, w, h, d);
  vxClearNoSVO(slb,0);
  return slb;
}

void vxFree(slab_t *slb) {
  vxRemesh(slb, 0);
  slb.svo.free;
  if (slb->info) free(slb->info);
  free(slb);
}

slab_t *vxClone(slab_t *slb) {
  slab_t *c = vxNew(slb->w,slb->h,slb->d);
  c.svo.clone(slb.svo);
  c->xyz = slb->xyz;
  c->center = slb->center;
  c->angle = slb->angle;
  c->scale = slb->scale;
  return c;
}

char *vxInfo(slab_t *slb) {
  return slb->info ? slb->info : "";
}

void vxSetInfo(slab_t *slb, char *info) {
  if (slb->info) free(slb->info);
  slb->info = strdup(info);
}

U32 vxW(slab_t *slb) { return slb->w; }
U32 vxH(slab_t *slb) { return slb->h; }
U32 vxD(slab_t *slb) { return slb->d; }


U32 vxGet(slab_t *slb, int x, int y, int z) {
  if (UNLIKELY((U32)x >= (U32)slb->w)) return NIL_COLOR;
  if (UNLIKELY((U32)y >= (U32)slb->h)) return NIL_COLOR;
  if (UNLIKELY((U32)z >= (U32)slb->d)) return NIL_COLOR;
  return slb.svo.get(x,y,z)->color;
}

//FIXME: vxSet clear normals of the neigboring voxels.
void vxSet(slab_t *slb, int x, int y, int z, U32 color) {
  if (UNLIKELY((U32)x >= (U32)slb->w)) return;
  if (UNLIKELY((U32)y >= (U32)slb->h)) return;
  if (UNLIKELY((U32)z >= (U32)slb->d)) return;
  if (color == NIL_COLOR) {
    slb.svo.erase(x,y,z);
  } else {
    attr_t *attr = slb.svo.set(x,y,z);
    attr->color = color;
    attr->normal = CLAY_NORMAL;
  }
}

int vxDbg(slab_t *slb, int new_dbg_state) {
  int prev = dbg;
  dbg = new_dbg_state;
  return prev;
}


void vxCamera(float fx, float fy, float fz  //focus point
             ,float ax, float ay, float az //angle
             ,float distance               //distance from focus point
             ,float scale                  //scale of result projection
             ,U32 screen_w
             ,U32 screen_h
) {
  gcam.focus = (vec3){fx, fy, fz};
  gcam.angle = (vec3){ax, ay, az};
  gcam.distance = distance;
  gcam.scale = scale;
  gcam.screen_w = screen_w;
  gcam.screen_h = screen_h;
}

void vxOrient(slab_t *slb, U32 flags
               ,float sx, float sy, float sz
               ,float cx, float cy, float cz
               ,float ax, float ay, float az
               ,float x, float y, float z
               ,float r, float g, float b

) {
  slb->flags = flags;
  slb->scale = (vec3){sx,sy,sz};
  slb->center = (vec3){cx,cy,cz};
  slb->angle = (vec3){ax,ay,az};
  slb->xyz = (vec3){x,y,z};
  slb->color = (vec3){r,g,b};
  if (slb->flags&SLB_FOCUS) { //hud items which are the satellites of the focus
    mat3 mw = mrotated(midentity(),vdeg2rad(gcam.angle));
    slb->xyz = vmm(slb->xyz,mw) + gcam.focus;
  }
}



typedef struct {
  vec3 xyz;
  vec3 angle;
  vec3 intensity; //intensity
} light_t;

CXLst(LightsList,light_t)
static LightsList scene_lights = CXLstInit;


typedef struct {
  vec3 min;
  vec3 max;
} aabb_t;

//we must decouple model view from slab_t, since we want different instancing
//for the same slb
typedef struct {
  slab_t *slb;
  Chunk *chunk;
  mat3 view;      //model view rotation and scale
  vec3 o;         //offset of the slab inside the scene (model view coords)
  vec3 scale;     //scale applied to model
  aabb_t aabb;    //AABB in the world space
  int index;      //our index on the scene items list
  mat3 rview;     //reverse model view rotation and scale
  vec3 center;
  vec3 angle;
  vec3 xyz;
  vec3 box;
  vec3 chunk_xyz;
} scene_item_t;


INLINE void rt_init(rt_t *rt, slab_t *slb) {
  rt->svo = slb.svo;
  rt->box = slb->box;
  rt->slb = slb;
}

INLINE void rt_t.init(scene_item_t *si) {
  //this->svo = slb.svo;
  this->box = si->box;
  this->slb = si->slb;
  this->chunk = si->chunk;
}

CXLst(SceneItems,scene_item_t)
static SceneItems scene_list = CXLstInit;

CXLst(ScenePItems,scene_item_t*)

typedef struct {
  aabb_t aabb[2]; //AABB for each kid
  int kids[2];
} bvh_node_t;

CXLst(BVHList,bvh_node_t)
static BVHList bvh = CXLstInit;

//FIXME: a items by their last time render hits
static int build_bvh_r(ScenePItems *RESTRICT items) {
  int i, j;

  scene_item_t **elts = items->elts;
  int node_index = (int)bvh.used;
  bvh_node_t *node = bvh.add;

  if (items.used == 2) {
    node->kids[0] = elts[0]->index;
    node->aabb[0] = elts[0]->aabb;
    node->kids[1] = elts[1]->index;
    node->aabb[1] = elts[1]->aabb;
    return -node_index;
  }
  vec3 bmin = (vec3){INFINITY,INFINITY,INFINITY};
  vec3 bmax = (vec3){-INFINITY,-INFINITY,-INFINITY};

  for (j = 0; j < items.used; j++) {
    scene_item_t *RESTRICT item = elts[j];
    bmin = vmin(bmin, item->aabb.min);
    bmax = vmax(bmax, item->aabb.max);
  }

  vec3 diags[][2] = {
    {bmin, bmax},
    {{bmin.x, bmax.y, bmin.z}, {bmax.x,bmin.y,bmax.z}},
    {{bmin.x, bmin.y, bmax.z}, {bmax.x,bmax.y,bmin.z}},
    {{bmax.x, bmin.y, bmin.z}, {bmin.x,bmax.y,bmax.z}}
  };

  aabb_t spl_aabb[2]; // splitted lists AABBs for different diagonals
  int k = -1;
  float best_dist = -INFINITY;

  for (i = 0; i < 4; i++) { //find the best split diagonal
    vec3 min0 = (vec3){INFINITY,INFINITY,INFINITY};
    vec3 max0 = (vec3){-INFINITY,-INFINITY,-INFINITY};
    vec3 min1 = (vec3){INFINITY,INFINITY,INFINITY};
    vec3 max1 = (vec3){-INFINITY,-INFINITY,-INFINITY};
    int cmin=0, cmax=0;
    for (j = 0; j < items.used; j++) {
      scene_item_t *RESTRICT item = elts[j];
      vec3 c = (item->aabb.min+item->aabb.max)/2.0f;
      float mind = distance(c,diags[i][0]);
      float maxd = distance(c,diags[i][1]);
      if (mind < maxd) {
        cmin++;
        min0 = vmin(min0, item->aabb.min);
        max0 = vmax(max0, item->aabb.max);        
      } else {
        cmax++;
        min1 = vmin(min1, item->aabb.min);
        max1 = vmax(max1, item->aabb.max);
      }
    }
    if (!(cmin && cmax)) continue;
    vec3 c0 = (min0+max0)/2.0f;
    vec3 c1 = (min1+max1)/2.0f;
    vec3 vd = c1-c0;
    float dist = dot(vd,vd);
    if (dist > best_dist) { //maximize distance between the centers of the split
      best_dist = dist;
      k = i;
      spl_aabb[0].min = min0;
      spl_aabb[0].max = max0;
      spl_aabb[1].min = min1;
      spl_aabb[1].max = max1;
    }
  }
  ScenePItems spl[2]; //items list split into two
  spl[0].init(items->used);
  spl[1].init(items->used);
  if (best_dist > -INFINITY) {
    for (j = 0; j < items.used; j++) {
      scene_item_t *RESTRICT item = elts[j];
      vec3 c = (item->aabb.min+item->aabb.max)/2.0f;
      float mind = distance(c,diags[k][0]);
      float maxd = distance(c,diags[k][1]);
      if (mind < maxd) {
        spl[0].push(item);       
      } else {
        spl[1].push(item);
      }
    }
  } else {
    // cant decide on split - split by middle
    // assuming user has passed the slabs in geometric relevance.
    vec3 min0 = (vec3){INFINITY,INFINITY,INFINITY};
    vec3 max0 = (vec3){-INFINITY,-INFINITY,-INFINITY};
    vec3 min1 = (vec3){INFINITY,INFINITY,INFINITY};
    vec3 max1 = (vec3){-INFINITY,-INFINITY,-INFINITY};
    int pivot = items.used/2;
    for (j = 0; j < items.used; j++) {
      scene_item_t *RESTRICT item = elts[j];
      if (j < pivot) {
        spl[0].push(item);       
        min0 = vmin(min0, item->aabb.min);
        max0 = vmax(max0, item->aabb.max);  
      } else {
        spl[1].push(item);
        min1 = vmin(min1, item->aabb.min);
        max1 = vmax(max1, item->aabb.max);
      }
    }
    spl_aabb[0].min = min0;
    spl_aabb[0].max = max0;
    spl_aabb[1].min = min1;
    spl_aabb[1].max = max1;
  }

  for (i = 0; i < 2; i++) {
    node->aabb[i] = spl_aabb[i];
    assert(spl[i].used != 0);
    if (spl[i].used == 1) {
      node->kids[i] = spl[i].elts[0]->index;
      continue;
    }
    node->kids[i] = build_bvh_r(&spl[i]);
    spl[i].free;
  }
  return -node_index;
}

//FIXME: sort BVH by distance to camera
static void build_bvh() {
  scene_item_t *RESTRICT scene_elts = scene_list.elts;
  ScenePItems items;
  items.init(scene_list.used);
  for (int j = 0; j < scene_list.used; j++) {
    scene_item_t *RESTRICT item = &scene_elts[j];
    item->index = j;
    items.push(item);
  }
  bvh.free;
  bvh.init(MAX(scene_list.used*2,10));
  build_bvh_r(&items);
  items.free;
}

static slab_t *dummy_slab = 0;

void vxBegin() {
  scene_list.free;
  scene_lights.free;

  //FIXME: pre alloc the same number as the previous number of slabs plus 5
  scene_list.init(10);
}

static scene_item_t slab_to_scene_item(slab_t *slb) {
  scene_item_t si;
  si.slb = slb;
  si.chunk = 0;
  si.view = mrotated(midentity(),vdeg2rad(slb->angle));
  si.scale = slb->scale;
  si.view = mscaled(si.view,1.0f/si.scale);
  si.o = slb->center + vmm(slb->xyz, si.view);

  mat3 idm = mscaled(midentity(),si.scale);
  si.rview = mrotated_r(idm,vdeg2rad(slb->angle));
  return si;
}

static scene_item_t
slab_chunk_to_scene_item(slab_t *slb, Chunk *cnk, vec3 xyz) {
  scene_item_t si;
  si.slb = slb;
  si.chunk = cnk;
  si.chunk_xyz = xyz;
  si.xyz = slb->xyz;
  si.center = slb->center-xyz;
  si.angle = slb->angle;
  si.scale = slb->scale;
  si.box = (vec3){32+EPS,32+EPS,32+EPS}; //FIXME: edge slabs will have dim%32

  si.view = mrotated(midentity(),vdeg2rad(si.angle));
  si.view = mscaled(si.view,1.0f/si.scale);
  si.o = si.center + vmm(si.xyz, si.view);

  mat3 idm = mscaled(midentity(),si.scale);
  si.rview = mrotated_r(idm,vdeg2rad(si.angle));
  return si;
}

void vxSlab(slab_t *slb) {
  if (slb->flags&SLB_LIGHT) {
    light_t light;
    light.xyz = -slb->xyz;
    light.angle = slb->angle;
    light.intensity = slb->color;
    scene_lights.push(light);
  }
  if (!(slb->flags&SLB_INVISIBLE)) {
    uint32_t count = 0;
    //vxSet(slb, 0,0,0, 0xFF000000);
    SVO_T *svo = &slb->svo;
    forEachChunk(cnk,svo) {
      //if (count == 63) break;
      ivec3 o = svo.chunk_xyz(count);
      scene_item_t si = slab_chunk_to_scene_item(slb, cnk, (vec3){o.x,o.y,o.z});
      scene_list.push(si);
      count++;
    }
  }
}

static void generate_aabbs() {
  scene_item_t *RESTRICT scene_elts = scene_list.elts;
  
  for (int j = 0; j < scene_list.used; j++) {
    scene_item_t *RESTRICT item = &scene_elts[j];
    mat3 idm = mscaled(midentity(),item->scale);
    mat3 mm = mrotated_r(idm,vdeg2rad(item->angle));

    vec3 bmin = (vec3){INFINITY,INFINITY,INFINITY};
    vec3 bmax = (vec3){-INFINITY,-INFINITY,-INFINITY};

    for (int i = 0; i < 8; i++) {
      vec3 corner = oct2gt[i];
      vec3 p = corner*item->box;
      //reverse the model rotation around center
      vec3 pw = vmm(p-item->center, mm)-item->xyz;
      bmin = vmin(bmin, pw);
      bmax = vmax(bmax, pw);
    }
    item->aabb.min = bmin;
    item->aabb.max = bmax;
  }
}

void vxEnd() {
  if (!dummy_slab) {
    dummy_slab = vxNew(1, 1, 1);
    vxClear(dummy_slab, NIL_COLOR);
    vxOrient(dummy_slab, 0
                       , 0.0, 0.0, 0.0, 0.5, 0.5, 0.5, 0.0, 0.0, 0.0
                       , 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  }
  while (scene_list.used < 2) vxSlab(dummy_slab);
  generate_aabbs();
  build_bvh();
}

//shading and surface hit info
typedef struct {
  float t;            //distance to surface from screen
  U32 voxel;     //voxel color
  slab_t *slb;        //slab which got hit
  vec3 n;             //surface normal
  vec3 p;             //xyz on surface
  U32 sx,sy;     //screen pixel x,y
  U32 sw,sh;     //screen w,h
  vec3 *prev_row;     //circular cache for surface points
  vec3 prev_col;      //two, because we need higher order derivative
} shd_t;


#define SHD_FLAT               0x0
#define SHD_POINTS             0x1
#define SHD_CUBES              0x2
#define SHD_ZBUFFER            0x3
#define SHD_NBUFFER            0x4
#define SHD_TEST               0x5
#define SHD_CUBIC_POINTS       0x6

#define SHD_SMOOTH       0x0100
#define SHD_LOWFI        0x0200
#define SHD_GLOW         0x0400
#define SHD_PILLOW       0x0800
#define SHD_ZCUE         0x1000
#define SHD_NEEDS_PREV   0x2000

#define SHD_TYPE(flags)  ((flags)&0xFF)
#define SHD_FLAGS(flags)  ((flags)&0xFFFFFF00)

static U32 shd_type;  //shader type
static U32 shd_flags;
typedef U32 (*shd_fn_t)(shd_t *RESTRICT s);
class !shd_fn_t; //hack
static shd_fn_t shd_fn;
static shd_fn_t shd_sky;
static U32 shd_sky_color;
static vec3 shd_ambient; //ambient light

#define SHD_VARS U32 color=s->voxel;float t=s->t; vec3 p=s->p;vec3 n=s->n; 

void vxAmbient(U32 sky_color, float r, float g, float b) {
  shd_sky_color = sky_color;
  shd_ambient = (vec3){r,g,b};
}

static U32 shd_points(shd_t *RESTRICT s) {
  SHD_VARS
  int i;
  if (UNLIKELY(s->slb->flags&SLB_LIGHT)) return s->voxel;
  light_t *RESTRICT lights = scene_lights.elts;
  vec3 l = shd_ambient;
  for (int i = 0; i < scene_lights.used; i++) {
    light_t *RESTRICT light = &lights[i];
    vec3 ldir = light->xyz - p;
    //angle should not be negative, otherwise we will get shadows
    //reducing ambient light.
    float angle = fmax(0.0f, vangle(ldir,n));
    l += angle*light->intensity;
  }
  U32 r,g,b;
  UNRGB(r,g,b,color);
  U32 rl = clamp_byte((int32_t)((float)r*l.x));
  U32 gl = clamp_byte((int32_t)((float)g*l.y));
  U32 bl = clamp_byte((int32_t)((float)b*l.z));
  return RGB(rl,gl,bl);
}

static U32 shd_nbuffer(shd_t *RESTRICT s) {
  return vnormal2rgb(s->n);
}

static U32 shd_zbuffer(shd_t *RESTRICT s) {
  float t = s->t;
  t -= BASE_DISTANCE;
  t = 230.0f-t/4.0;
  return v2rgb((vec3){t,t,t});
}

static U32 shd_flat(shd_t *RESTRICT s) {
  return s->voxel;
}

//this shader replace surface normal with the zbuffer derived cube face normal
static U32 shd_cubes(shd_t *RESTRICT s) {
  SHD_VARS
  U32 result = 0x7f7f7f;
  U32 x = s->sx, y = s->sy;
  if (!(x > 0 && y > 0)) goto end;

  vec3 q = s->prev_col    - p;
  vec3 r = s->prev_row[x] - p;
  vec3 en = normalized(cross(q,r));
  s->n = en;
  result = shd_points(s);
  s->n = n;

end:
  s->prev_col = p;
  s->prev_row[x] = p;
  return result;
}

static U32 shd_cubic_points(shd_t *RESTRICT s) {
  s->voxel = shd_cubes(s);
  U32 x = s->sx, y = s->sy;
  if (!(x > 0 && y > 0)) return s->voxel;
  s->voxel = shd_points(s);
  return s->voxel;
}

static U32 shd_sky_transparent(shd_t *RESTRICT s) {
  return shd_sky_color;
}

static U32 shd_sky_store_prev(shd_t *RESTRICT s) {
  vec3 inf = (vec3){INFINITY,INFINITY,INFINITY};
  s->prev_col = inf;
  s->prev_row[s->sx] = inf;
  if (!s->sx || !s->sy) return 0x7f7f7f;
  return shd_sky_color;
}

void vxShader(U32 flags) {
  shd_type = SHD_TYPE(flags);
  shd_flags = flags;
  shd_fn = &shd_points;
  shd_sky = &shd_sky_transparent;
  if (shd_type == SHD_NBUFFER) shd_fn = &shd_nbuffer;
  else if (shd_type == SHD_ZBUFFER) shd_fn = &shd_zbuffer;
  else if (shd_type == SHD_FLAT) shd_fn = &shd_flat;
  else if (shd_type == SHD_CUBES) {
    shd_fn = &shd_cubes;
    shd_flags |= SHD_NEEDS_PREV;
    shd_sky = &shd_sky_store_prev;
  } else if (shd_type == SHD_CUBIC_POINTS) {
    shd_fn = &shd_cubic_points;
    shd_flags |= SHD_NEEDS_PREV;
    shd_sky = &shd_sky_store_prev;
  }
}



static char slb_sample_txt[1024];

char *vxSampleAABB(slab_t *slb) {
#include "camera_setup.h"

  sxp = syp;

  scene_item_t si = slab_to_scene_item(slb);
  scene_item_t *RESTRICT item = &si;
  vec3 rom = vmm(sxp,item->view);
  vec3 rdm = normalized((vmm(szb*PRECSCL+sxp,item->view) - rom)/PRECSCL);
  rom += item->o;

  vec3 screen_dir = -rdm;

  int gw = cam.screen_w;
  int gh = cam.screen_h;
  int gw1 = gw-1;
  int gh1 = gh-1;
  float gw05 = gw*0.5f;
  float gh05 = gh*0.5f;

  mat3 idm = mscaled(midentity(),item->scale);
  mat3 mm = mrotated_r(idm,vdeg2rad(slb->angle));
  mat3 mw = mrotated_r(midentity(),wrot);

  mw.X *= -cam.scale*cam.screen_w;
  mw.Y *=  cam.scale*cam.screen_h;

  char cs[8][64];

  for (int i = 0; i < 8; i++) {
    vec3 corner = oct2gt[i];
    vec3 p = corner*slb->box;
    
    //ray plane hit always succeeds here
    vec3 hit = ray_plane_hit(p, screen_dir, rom, rdm);
    //reverse the model rotation around center
    vec3 hitw = vmm(hit-slb->center, mm)-slb->xyz;
    vec3 sp = vmm(hitw-cam.focus, mw); //voxel p projected onto screen
    sp += (vec3){gw05, gh05, 0.0f};
    
    sprintf(cs[i], "%f %f %f", sp.x, sp.y, sp.z);
  }

  sprintf(slb_sample_txt, "%s %s %s %s %s %s %s %s"
         ,cs[0], cs[1], cs[2], cs[3]
         ,cs[4], cs[5], cs[6], cs[7]);

  return slb_sample_txt;
}

char *vxSampleWorldAABB(slab_t *slb) {
  vec3 mrot = vdeg2rad(slb->angle);
  mat3 mm = mscaled(mrotated_r(midentity(),mrot),slb->scale);

  vec3 bmin = (vec3){INFINITY,INFINITY,INFINITY};
  vec3 bmax = (vec3){-INFINITY,-INFINITY,-INFINITY};

  for (int i = 0; i < 8; i++) {
    vec3 corner = oct2gt[i];
    vec3 pm = corner*slb->box;
    vec3 pw = vmm(pm-slb->center,mm) + slb->xyz;
    bmin = vmin(bmin, pw);
    bmax = vmax(bmax, pw);
  }

  sprintf(slb_sample_txt, "%f %f %f %f %f %f"
         ,bmin.x, bmin.y, bmin.z
         ,bmax.x, bmax.y, bmax.z);
  return slb_sample_txt;
}

#define PASTE_COLOR
slab_t *vxCopyBox(slab_t *slb, int x0, int y0, int z0, int x1, int y1, int z1) {
  int w = x1-x0;
  int h = y1-y0;
  int d = z1-z0;
  slab_t *c = vxNew(w,h,d);
  vxClear(c, NIL_COLOR);

  U32 paste_color;

  U32 x,y,z;
  SVO_T *svo = c.svo;
  SVOTerms *terms = vxListTerms(c);
  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    U32 color = slb.svo.term_color(t);
    U32 sx = t->o.x;
    U32 sy = t->o.y;
    U32 sz = t->o.z;
    U32 c = t->c;
    U32 cuts_count = 0;
    U32 voxel_count = 0;
    for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
      for (x = sx; x < sx+c; x++) {
        int xx=x+x0;
        int yy=y+y0;
        int zz=z+z0;
        if (!(x0 <= xx && xx < x1
           && y0 <= yy && yy < y1
           && z0 <= zz && zz < z1)) {
          continue;
        }
        paste_color = vxGet(slb, xx, yy, zz);
        U32 cut = paste_color != NIL_COLOR;
#include "cut_body.h"
      }
    }
#include "cut_tail.h"
  }
  delete(terms);
  vxRemesh(c, 0);
  vxNormalizeBox(c);

  return c;
}
#undef PASTE_COLOR

char *vxMargins(slab_t *slb) {
  SVOTerms *terms = vxListTerms(slb);

  vec3 bmin = (vec3){INFINITY,INFINITY,INFINITY};
  vec3 bmax = (vec3){-INFINITY,-INFINITY,-INFINITY};

  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    if (!slb.svo.exists(t)) continue;
    vec3 omin = (vec3){t->o.x, t->o.y, t->o.z};
    U32 c = t->c;
    vec3 omax = (vec3){t->o.x+c, t->o.y+c, t->o.z+c};
    bmin = vmin(bmin, omin);
    bmax = vmax(bmax, omax);
  }
  delete(terms);

  sprintf(slb_sample_txt, "%d %d %d %d %d %d"
         ,(int)roundf(bmin.x), (int)roundf(bmin.y), (int)roundf(bmin.z)
         ,(int)roundf(bmax.x), (int)roundf(bmax.y), (int)roundf(bmax.z));

  return slb_sample_txt;
}

//ther was a bug, where large dist broke the raycast algorithm
//#define RB_DIST 1500.0f
#define RB_DIST 100000.0f
//rd*sqrt(3) + margin to get out int empty space
#define RB_MARGIN (1.732051f+0.5f)

#define RB_CENTER (vec3){0.5f,0.5f,0.5f}

#define RB_MIN (vec3){0.0f,0.0f,0.0f}
#define RB_MAX (vec3){1.0f,1.0f,1.0f}

#define RB_C0 (vec3){1.0f,1.0f,0.0f}
#define RB_C1 (vec3){0.0f,1.0f,1.0f}
#define RB_C2 (vec3){1.0f,0.0f,1.0f}
#define RB_C3 (vec3){1.0f,0.0f,0.0f}
#define RB_C4 (vec3){0.0f,1.0f,0.0f}
#define RB_C5 (vec3){0.0f,0.0f,1.0f}

INLINE int ray_blocked(slab_t *slb, vec3 ro, vec3 rd) {
  rt_t rtt, *rt = &rtt;
  rt_init(rt, slb);
  ro += rd*RB_DIST;
  rt->rd = -rd;
#define RB_TEST rt.shot; if (rt->t>=RB_DIST-RB_MARGIN) return 0;
  rt->ro = ro; RB_TEST
  rt->ro = ro+RB_MAX; RB_TEST
  rt->ro = ro+RB_CENTER; RB_TEST
  rt->ro = ro+RB_C0; RB_TEST
  rt->ro = ro+RB_C1; RB_TEST
  rt->ro = ro+RB_C2; RB_TEST
  rt->ro = ro+RB_C3; RB_TEST
  rt->ro = ro+RB_C4; RB_TEST
  rt->ro = ro+RB_C5; RB_TEST
  return 1;
#undef RB_TEST
}

void vxProject(slab_t *slb, gfx_t *gfx, int cut, int texture, int invert) {
#include "camera_setup.h"

  sxp = syp;

  scene_item_t si = slab_to_scene_item(slb);
  scene_item_t *RESTRICT item = &si;
  vec3 rom = vmm(sxp,item->view);
  vec3 rdm = normalized((vmm(szb*PRECSCL+sxp,item->view) - rom)/PRECSCL);
  rom += item->o;

  vec3 screen_dir = -rdm;

  int gw = gfx->w;
  int gh = gfx->h;
  int gw1 = gw-1;
  int gh1 = gh-1;
  float gw05 = gw*0.5f;
  float gh05 = gh*0.5f;

  mat3 idm = mscaled(midentity(),item->scale);
  mat3 mm = mrotated_r(idm,vdeg2rad(slb->angle));
  mat3 mw = mrotated_r(midentity(),wrot);

  mw.X *= -cam.scale*cam.screen_w;
  mw.Y *=  cam.scale*cam.screen_h;

  U32 *pixels = gfx->data;
  SVO_T *svo = slb.svo;

#ifdef PROFILE
  float TStart = nano_clock();
#endif

  if (texture) {
    SVOTerms *shd_terms = 0;
    SVOTerms *terms = vxMeshTerms(slb);

    //fprintf(stderr,"terms: %d/%d\n",terms->used,terms->size);
    for (int i = 0; i < terms->used; i++) {
      term_t *t = terms.ref(i);
      U32 x = t->o.x;
      U32 y = t->o.y;
      U32 z = t->o.z;
      vec3 p = {x,y,z}; //currently processed voxel x,y,z

#define CONTINUE_STMNT continue
#include "project_body.h"
#undef CONTINUE_STMNT

      if (!transparent) {
        //if (ray_blocked(slb,p,screen_dir)) continue;
        if (cut) {
          if (cut==3) {
            svo.term_erase(t);
          } else { //cut==4 or cut==5
            int dx,dy,dz;
            if (cut==5) { //grow
              vec3 q = p;
              do {
                q += screen_dir;
              } while ((U32)q.x == x
                      && (U32)q.y == y
                      && (U32)q.z == z);
              vxSet(slb, (int)q.x, (int)q.y, (int)q.z, pixel);
              return;
            } 
            for (dy = -1; dy < 2; dy++) {
              for (dx = -1; dx < 2; dx++) {
                for (dz = -1; dz < 2; dz++) {
                  vxSet(slb, (int)x+dx, (int)y+dy, (int)z+dz, pixel);
                }
              }
            }
          }
        } else {
          if (t->c == 1) {
            svo.term_set_mesh(t,pixel);
          } else {
            svo.term_split(t,terms);
          }
        }
      }
    }
    if (cut && texture==2) {
      vxRemesh(slb, 0);
    }
    return;
  }

  //cut, no texture
  SVOTerms *terms = vxListTerms(slb);
  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    if (!svo.exists(t)) continue; //nothing to project at
    U32 sx = t->o.x;
    U32 sy = t->o.y;
    U32 sz = t->o.z;
    U32 c = t->c;
    U32 cuts_count = 0;
    U32 voxel_count = 0;
    //FIXME: a better solution for large cubes: first projecting shadow the cube
    //       to the screen then checking its pixels against to see if any hit.
    U32 x,y,z;
    vec3 p; //currently processed voxel x,y,z
    for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
      p.y = (float)y;
      p.z = (float)z;
      for (x = sx; x < sx+c; x++) {
        p.x = (float)x;
        voxel_count++;
#define CONTINUE_STMNT goto cut_miss
#include "project_body.h"
#undef CONTINUE_STMNT
        U32 cut = transparent;
        if (invert) cut = !cut;

#include "cut_body.h"
  cut_miss:
        if (cuts_count) {
          goto do_split_term;
        }
      }
    }
#include "cut_tail.h"
  }
  delete(terms);
  if (cut == 1) {
    vxRemesh(slb, 0);
  }

#ifdef PROFILE
  float TEnd = nano_clock();
  fprintf(stderr, "vxProject time: %g seconds\n", TEnd-TStart);
#endif
}


char *vxSamplePastedAABB(slab_t *slb, slab_t *brush) {
  vec3 bmin = (vec3){0.0f,0.0f,0.0f};
  vec3 bmax = (vec3){slb->w-1,slb->h-1,slb->d-1};

  //we will be transfering from brush into slab coords
  slab_t *t = brush;
  brush = slb;
  slb = t;

  vec3 mrot = vdeg2rad(slb->angle);
  vec3 brot = vdeg2rad(brush->angle);

  mat3 idm = midentity();
  mat3 mm = mscaled(mrotated_r(idm,mrot),slb->scale);
  mat3 mb = mscaled(mrotated(midentity(),brot),1.0f/brush->scale);

  SVOTerms *terms = vxListTerms(slb);
  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    U32 color = slb.svo.term_color(t);
    if (!color) continue; //empty paste
    U32 sx = t->o.x;
    U32 sy = t->o.y;
    U32 sz = t->o.z;
    U32 c = t->c;
    U32 x,y,z;
    for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
      for (x = sx; x < sx+c; x++) {
        vec3 po, pb;
        vec3 pm = (vec3){(float)x,(float)y,(float)z};
        po = vmm(pm-slb->center,mm) - slb->xyz; //model coords reverse
        pb = vmm(po+brush->xyz,mb) + brush->center; //move to brush coords
        int xx=roundf(pb.x),yy=roundf(pb.y),zz=roundf(pb.z);
        vec3 pb2 = (vec3){xx,yy,zz};
        bmin = vmin(bmin, pb2);
        bmax = vmax(bmax, pb2+(vec3){1.0f,1.0f,1.0f});
      }
    }
  }
  delete(terms);
  sprintf(slb_sample_txt, "%f %f %f %f %f %f"
         ,bmin.x, bmin.y, bmin.z
         ,bmax.x, bmax.y, bmax.z);
  return slb_sample_txt;
}



#define PASTE_COLOR
void vxPaste(slab_t *slb, slab_t *brush, U32 flags) {
  U32 invert = flags&0x01;
  U32 paste_color;

  vec3 mrot = vdeg2rad(slb->angle);
  vec3 brot = vdeg2rad(brush->angle);

  mat3 idm = midentity();
  mat3 mm = mscaled(mrotated_r(idm,mrot),slb->scale);
  mat3 mb = mscaled(mrotated(midentity(),brot),1.0f/brush->scale);

  U32 x,y,z;
  SVO_T *svo = slb.svo;
  SVOTerms *terms = vxListTerms(slb);
  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    U32 color = slb.svo.term_color(t);
    U32 sx = t->o.x;
    U32 sy = t->o.y;
    U32 sz = t->o.z;
    U32 c = t->c;
    U32 cuts_count = 0;
    U32 voxel_count = 0;
    for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
      for (x = sx; x < sx+c; x++) {
        vec3 po, pb;
        vec3 pm = (vec3){(float)x,(float)y,(float)z};
        po = vmm(pm-slb->center,mm) - slb->xyz; //model coords reverse
        pb = vmm(po+brush->xyz,mb) + brush->center; //move to brush coords
        int xx=roundf(pb.x),yy=roundf(pb.y),zz=roundf(pb.z);
        paste_color = vxGet(brush, xx, yy, zz);
        U32 cut = paste_color != NIL_COLOR;
        if (invert) cut = !cut;
#include "cut_body.h"
      }
    }
#include "cut_tail.h"
  }
  delete(terms);
  vxRemesh(slb, 0);
  vxNormalizeBox(slb);
}
#undef PASTE_COLOR

void vxCut(slab_t *slb, slab_t *brush, U32 flags) {
  U32 invert = flags&0x01;

  vec3 mrot = vdeg2rad(slb->angle);
  vec3 brot = vdeg2rad(brush->angle);

  mat3 idm = midentity();
  mat3 mm = mscaled(mrotated_r(idm,mrot),slb->scale);
  mat3 mb = mscaled(mrotated(midentity(),brot),1.0f/brush->scale);

  U32 x,y,z;
  SVO_T *svo = slb.svo;
  SVOTerms *terms = vxListTerms(slb);
  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    if (!slb.svo.exists(t)) continue; //nothing to cut
    U32 color = slb.svo.term_color(t);
    U32 sx = t->o.x;
    U32 sy = t->o.y;
    U32 sz = t->o.z;
    U32 c = t->c;
    U32 cuts_count = 0;
    U32 voxel_count = 0;
    for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
      for (x = sx; x < sx+c; x++) {
        vec3 po, pb;
        vec3 pm = (vec3){(float)x,(float)y,(float)z};
        po = vmm(pm-slb->center,mm) - slb->xyz; //model coords reverse
        pb = vmm(po+brush->xyz,mb) + brush->center; //move to brush coords
        int xx=roundf(pb.x),yy=roundf(pb.y),zz=roundf(pb.z);
        U32 cut = vxGet(brush, xx, yy, zz) != NIL_COLOR;
        if (invert) cut = !cut;
#include "cut_body.h"
      }
    }
#include "cut_tail.h"
  }
  delete(terms);
  vxRemesh(slb, 0);
}

//cut everything outside of the box
static void cut_box(slab_t *slb, int invert
                  , U32 bx, U32 by, U32 bz
                  , U32 bw, U32 bh, U32 bd) {
  U32 x,y,z;
  SVO_T *svo = slb.svo;
  SVOTerms *terms = vxListTerms(slb);
  for (int i = 0; i < terms->used; i++) {
    term_t *t = terms.ref(i);
    if (!slb.svo.exists(t)) continue; //nothing to cut
    U32 sx = t->o.x;
    U32 sy = t->o.y;
    U32 sz = t->o.z;
    U32 c = t->c;
    U32 cuts_count = 0;
    U32 voxel_count = 0;
    for (z = sz; z < sz+c; z++) for (y = sy; y < sy+c; y++) {
      for (x = sx; x < sx+c; x++) {
        U32 cut = x>=bx && x<bw && y>=by && y<bh && z>=bz && z<bd;
        if (invert) cut = !cut;
#include "cut_body.h"
      }
    }
#include "cut_tail.h"
  }
  delete(terms);
  vxRemesh(slb, 0);
}

//#define BVH_COUNT

#ifdef BVH_COUNT
double nrobjs;
double nhits;
#endif

void ray_bvh_hit(int n, shd_t *RESTRICT hit, vec3 ro, vec3 rd) {
  bvh_node_t *node = bvh.ref(-n);

  int order[2];
  vec2 ts[2];
  ts[0] = ray_box_hit_test(ro, rd, node->aabb[0].min, node->aabb[0].max);
  ts[1] = ray_box_hit_test(ro, rd, node->aabb[1].min, node->aabb[1].max);
  if (ts[0].x > ts[1].x) {
    vec2 t = ts[0];
    ts[0] = ts[1];
    ts[1] = t;
    order[0] = 1;
    order[1] = 0;
  } else {
    order[0] = 0;
    order[1] = 1;
  }

  if (!(ts[0].x < INFINITY)) return; //both ray-box tests are misses.

  for (int i = 0; i < 2; i++) {
    int o = order[i];
    int kid = node->kids[o];
    if (kid<0) { //non-leaf?
      ray_bvh_hit(kid, hit, ro, rd);
      goto next;
    }
    scene_item_t *RESTRICT item = &scene_list.elts[kid];
    //FIXME: rd*PRECSCL and ro/PRECSCL is a hack to avoid precision loss in rd
    vec3 rom = vmm(ro,item->view);
    vec3 rdm = (vmm(rd*PRECSCL+ro,item->view) - rom)/PRECSCL;
    rom += item->o;
    rt_t rtt, *rt = &rtt;
    rt.init(item);
    rt->ro = rom;
    rt->rd = rdm;
#ifdef BVH_COUNT
    nrobjs++;
#endif
    rt.shot;
    float t = rt->t;
    if (t < hit->t) {
#ifdef BVH_COUNT
      nhits++;
#endif
      hit->t = t;
      attr_t *attr = rt->attr;
      hit->voxel = attr->color;
      slab_t *slb = item->slb;
      hit->slb = slb;
      vec3 p = rdm*(rt->t+0.001f) + rom; //FIXME: still produces tiny black dots
      p += item->chunk_xyz;
      vec3 n;
      if (attr->normal == MESH_NORMAL) {
        n = get_normal(slb, p);
        attr->normal = vnormal2rgb(n);
      } else {
        n = vrgb2normal(attr->normal);
      }
      //vec3 nm = ray_plane_hit(p+n, -rdm, rom, rdm); //project onto screen
      hit->n = vmm(n,item->rview); //direction can be just rotated
      /*vec3 nm = rom+n;
      vec3 nw = vmm(nm-slb->center, item->rview)-slb->xyz; //to world
      hit->n = nw-ro;*/

    }
  next:
    if (hit->t <= ts[1].x) return; //hit before the second box.
    if (ts[1].x == INFINITY) return; //missed the second box
  }
}

//FIXME: consider sorting items by the distance towards screen.
void vxRender(gfx_t *cbuf) {
#include "camera_setup.h"

#ifdef BVH_COUNT
  nhits = 0.0;
  nrobjs = 0.0;
#endif

  SVO_PRF(ot_rcnt=0); SVO_PRF(ot_vcnt=0);
#ifdef PROFILE
  double TStart = nano_clock();
#endif

  /*mat3 rworld = mrotated_r(midentity(),wrot);
  rworld.X *= -cam.scale*cam.screen_w;
  rworld.Y *=  cam.scale*cam.screen_h;*/

  shd_t s;
  s.sw = sw;
  s.sh = sh;

  if (shd_flags&SHD_NEEDS_PREV) {
    s.prev_row = malloc(sizeof(vec3)*4*1024);
  }

  U32 *colors = cbuf->data;

  for (s.sy = 0; s.sy < sh; s.sy++) { //iterate over screen x,y in world space
    U32 *pcs = colors + s.sy*sw;
    sxp = syp;
    for (s.sx = 0; s.sx < sw; s.sx++) {
      //ray_sx = s.sx;
      //ray_sy = s.sy;
      s.t = INFINITY;
      ray_bvh_hit(0, &s, sxp, szb);
      if (s.t < INFINITY) {
        s.p = sxp + s.t*szb;
        *pcs = shd_fn(&s);
      } else {
        *pcs = shd_sky(&s);
      }
      pcs++;
      sxp -= scr.xb;
      bug = 0;
    }
    syp += scr.yb;
  }
  if (shd_flags&SHD_NEEDS_PREV) {
    free(s.prev_row);
  }
  SVO_PRF(fprintf(stderr, "vxRender: rays=%d, voxels=%d vpr=%g\n"
                , rcnt, vcnt, (float)vcnt/(float)rcnt));

#ifdef BVH_COUNT
  fprintf(stdout, "objects per ray: %g\n", nrobjs/nhits);
#endif

#ifdef PROFILE
  double TEnd = nano_clock();
  double TPassed = TEnd-TStart; 
  //fprintf(stderr, "camera: "); VPRINT(cam.angle);
  fprintf(stderr, "vxRender time: %g seconds\n", TPassed);
#endif
}

char *vxSampleRay(slab_t *slb, U32 scx, U32 scy) {
#include "camera_setup.h"
#if 1
  sprintf(slb_sample_txt, "void");
  return slb_sample_txt;
#endif

  sxp = syp;
  sxp -= scr.xb*(float)scx;
  sxp += scr.yb*(float)scy;

  scene_item_t si = slab_to_scene_item(slb);
  scene_item_t *RESTRICT item = &si;

  vec3 ro = sxp;
  vec3 rd = szb;

  vec3 rom = vmm(ro,item->view);
  vec3 rdm = (vmm(rd*PRECSCL+ro,item->view) - rom)/PRECSCL;
  rom += item->o;
  rt_t rtt, *rt = &rtt;
  rt_init(rt, slb);
  rt->ro = rom;
  rt->rd = rdm;
  rt.shot;
  if(!rt->attr) {
    sprintf(slb_sample_txt, "void");
    return slb_sample_txt;
  }
  vec3 hit = rdm*(rt->t+0.01f) + rom;
  sprintf(slb_sample_txt, "%f %f %f #%x",hit.x,hit.y,hit.z
         ,rt->attr->color&0xFFFFFF);
  return slb_sample_txt;
}


U32 vxRenderSample(int scx, int scy) {
#include "camera_setup.h"
  sxp = syp;
  sxp -= scr.xb*(float)scx;
  sxp += scr.yb*(float)scy;

  shd_t s;
  s.sw = sw;
  s.sh = sh;

  if (shd_flags&SHD_NEEDS_PREV) {
    s.prev_row = malloc(sizeof(vec3)*4*1024);
  }

  U32 sample;

  s.t = INFINITY;
  ray_bvh_hit(0, &s, sxp, szb);
  if (s.t < INFINITY) {
    s.p = sxp + s.t*szb;
    sample = shd_fn(&s);
  } else {
    sample = shd_sky(&s);
  }

  if (shd_flags&SHD_NEEDS_PREV) {
    free(s.prev_row);
  }
  return sample;
}


//also save orientation: scale, center, angle, xyz
static char vxz_header[] = {'v','x', 'z', 7, 11, 2, '0','1'};

void vxSave(char *filename, slab_t *slb) {
  FILE *out;
  out = fopen(filename, "wb");
  if (!out) return;
  fwrite(vxz_header, 1, 8, out);
  fwrite(&slb->w, 1, 4, out);
  fwrite(&slb->h, 1, 4, out);
  fwrite(&slb->d, 1, 4, out);
  fwrite(&slb->flags, 1, 4, out);
  char *info = vxInfo(slb);
  U32 infolen = strlen(info);
  fwrite(&infolen, 1, 4, out);
  fwrite(info, 1, infolen, out);
  slb.svo.file_save(out);
  //slb->svo.file_save(out);
  fclose(out);
}

slab_t *vxLoad(char *filename) {
  U8 buf[512];
  U32 w,h,d,flags,infolen;
  FILE *in = 0;
  char *info;
  slab_t *slb = 0;
  in = fopen(filename, "rb");
  if (!in) goto fail;
  fread(buf, 1, 8, in);
  if (memcmp(buf,vxz_header,8)) goto fail; //bad header
  fread(&w, 1, 4, in);
  fread(&h, 1, 4, in);
  fread(&d, 1, 4, in);
  fread(&flags, 1, 4, in);
  fread(&infolen, 1, 4, in);
  info = (char*)malloc(infolen+1);
  fread(info, 1, infolen, in);
  info[infolen] = 0;
  //fprintf(stderr, "whd=%dx%dx%d\n",w,h,d);
  //fprintf(stderr, "info: %s\n",info);
  slb = vxNew(w,h,d);
  vxRemesh(slb, 0);
  slb.svo.file_load(in);
  fclose(in);
  vxRecalcNormals(slb);
  vxSetInfo(slb, info);
  return slb;
fail:
  if (in) fclose(in);
  if (slb) free(slb);
  return 0;
}

@end
