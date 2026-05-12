#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <gfx.h>

#:nc
#:vec3


class !gfx_t;

//octant coordinate lists
static vec3 oct2gt[] = {
  {0.0,0.0,0.0},{1.0,0.0,0.0},
  {0.0,1.0,0.0},{1.0,1.0,0.0},
  {0.0,0.0,1.0},{1.0,0.0,1.0},
  {0.0,1.0,1.0},{1.0,1.0,1.0}
};

static ivec3 ioct2gt[] = {
  {0,0,0},{1,0,0},
  {0,1,0},{1,1,0},
  {0,0,1},{1,0,1},
  {0,1,1},{1,1,1}
};

static vec3 dirs3d6[] = {
  { 1,0,0},{0, 1,0},{0,0, 1},
  {-1,0,0},{0,-1,0},{0,0,-1},
};

static ivec3 idirs3d6[] = {
  { 1,0,0},{0, 1,0},{0,0, 1},
  {-1,0,0},{0,-1,0},{0,0,-1},
};

//directions in 3d space, including diagonals
#DIRS3D_COUNT 26
static vec3 dirs3d[] = {
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

static ivec3 idirs3d[] = {
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

#if 0
//normalized directions
static vec3 ndirs3d[DIRS3D_COUNT] = {
   {-0.577350,-0.577350,-0.577350}
  ,{-0.707107,-0.707107,0.000000}
  ,{-0.577350,-0.577350,0.577350}
  ,{-0.707107,0.000000,-0.707107}
  ,{-1.000000,0.000000,0.000000}
  ,{-0.707107,0.000000,0.707107}
  ,{-0.577350,0.577350,-0.577350}
  ,{-0.707107,0.707107,0.000000}
  ,{-0.577350,0.577350,0.577350}
  ,{0.000000,-0.707107,-0.707107}
  ,{0.000000,-1.000000,0.000000}
  ,{0.000000,-0.707107,0.707107}
  ,{0.000000,0.000000,-1.000000}
  ,{0.000000,0.000000,1.000000}
  ,{0.000000,0.707107,-0.707107}
  ,{0.000000,1.000000,0.000000}
  ,{0.000000,0.707107,0.707107}
  ,{0.577350,-0.577350,-0.577350}
  ,{0.707107,-0.707107,0.000000}
  ,{0.577350,-0.577350,0.577350}
  ,{0.707107,0.000000,-0.707107}
  ,{1.000000,0.000000,0.000000}
  ,{0.707107,0.000000,0.707107}
  ,{0.577350,0.577350,-0.577350}
  ,{0.707107,0.707107,0.000000}
  ,{0.577350,0.577350,0.577350}
};
#endif

typedef struct {
  U32 color;
  U32 normal;
} attr_t;

TLst(atrs_t,attr_t)


//normal for empty voxels
#NIL_NORMAL  0x80FFFF00

//normal for either surface or occluded voxels
#CLAY_NORMAL 0x807F7F00

//normal for surface voxels
#MESH_NORMAL 0x80000000
  
// softness mask for normals
#SOFTNESS_MASK 0x0F000000


// flags for locked mesh editing
#MESH_LOCK 0x10000000


//color for empty voxels
#NIL_COLOR 0xFF000000

inline int attr_t.is_mesh { //true if voxel is part of surface
  return this->normal <= MESH_NORMAL;
}

inline int attr_t.is_empty { //true if voxel is empty
  return this->color == NIL_COLOR;
}


struct slab_t;
typedef struct slab_t slab_t;

struct svo_t;
typedef struct svo_t svo_t;

struct ot_t;
typedef struct ot_t ot_t;

//information for the raytracer
typedef struct {
  ot_t *RESTRICT svo;
  vec3 ro;
  vec3 rd;
  vec3 ird;
  float t;
  attr_t attr;
  vec3 box;
} rt_t;

#CAM_PERSPECTIVE   0x01


typedef struct {
  vec3 ro; //postion
  vec3 xb; //right direction
  vec3 yb; //up direction
  vec3 zb; //forward direction
  float scale;
  float focal_length;
  U32 screen_w;
  U32 screen_h;
  U32 flags;
} camera_t;

//base camera distance where screen culling happens
//larger distance allows larger models
//#BASE_DISTANCE 1000.0
//#BASE_DISTANCE 100.0
//#BASE_DISTANCE 10.0
#BASE_DISTANCE 0.0

//#FOCUS_STRENGTH 200.0
//#FOCUS_STRENGTH 150.0

#DEFAULT_FOCAL_LENGTH 200.0

//rd*PRECSCL and rdm/PRECSCL is a hack to avoid precision loss in rd
//#PRECSCL BASE_DISTANCE
#PRECSCL 1.0


#:profile
#:dbg
