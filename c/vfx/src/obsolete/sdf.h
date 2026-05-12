//origin https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
//vec3 q = abs(p) - b;
//return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
static float sd_cube(vec3 p, float edgesz) {
  edgesz *= 0.5;
  vec3 b = (vec3){edgesz, edgesz, edgesz};
  vec3 q = vabs(p) - b;
  vec3 o = {0.0,0.0,0.0};
  return vlen(vmax(q, o)) + MIN(mine(q),0.0);
}

#define OT_VOXEL(n) ((n)&0xFFFFFF)
#define SQRT3 1.7321


#define SDOT_DCUT SQRT3
//#define SDOT_DCUT 1.0
//#define SDOT_DCUT (c*2.0)


static INLINE
void sd_octree(uint32_t *color, float *sd, otree_t *RESTRICT tree
          ,int node, float c
          ,float x, float y, float z
          ,float ox, float oy, float oz)
{
  uint32_t *octs = lst_get(tree,node).octs;
  int oct;
  for (oct = 0; oct < 8; oct++) {
    uint32_t next = octs[oct];
    uint32_t voxel = OT_VOXEL(next);
    if (!voxel) continue; //empty
    float oxx = ox + c*((oct   )&1);
    float oyy = oy + c*((oct>>1)&1);
    float ozz = oz + c*((oct>>2)&1);
    float xx = x-oxx;
    float yy = y-oyy;
    float zz = z-ozz;
    vec3 p = (vec3){xx, yy, zz};
    float tsd = sd_octant(p,c);
    if (next&OT_TERM) { //term: exact distance
      if (tsd < *sd) {
        *sd = tsd;
        *color = voxel;
      }
      continue;
    }
    if (tsd > SDOT_DCUT) { //too far: no need to dig further
      if (tsd+SDOT_DCUT < *sd) {
        *sd = tsd+SDOT_DCUT; //save just the distance to it
        *color = voxel;
      }
      continue;
    }
    sd_octree(color, sd, tree, next, c*0.5, x, y, z, oxx, oyy, ozz);
  }
}

#define MIN_DIST 0.1

//FIXME: currently we have problem traversing empty distances
//       these result in improper large distance jumps or no distance at all
float vfx_sd(vfx_t *vfx, uint32_t *color, vec3 p) {
  float c = vfx->c;
  float sd = 999999999999.0;
  *color = 0;
  sd_octree(color, &sd, &vfx->tree, 0, c, p.x, p.y, p.z, 0.0, 0.0, 0.0);
  /*if (!*color) {
    sd = sd_octant(p,c*2.0);
    fprintf(stderr, "hello\n");
    if (sd < 0.0) {
      return SDOT_DCUT;
    }
    return sd+MIN_DIST;
  }*/
  return sd;
}

uint32_t raymarch(vfx_t *RESTRICT vfx
                 ,vec3 *RESTRICT hit
                 ,vec3 *RESTRICT ro_arg
                 ,vec3 *RESTRICT rd_arg)
{
  int i, j;
  vec3 ro = *ro_arg;
  vec3 rd = *rd_arg;
  vec3 r = ro;
  vec3 b = {vfx->c, vfx->c, vfx->c};

  uint32_t color = 0;
  for (i = 0; i < 40; i++) {
    float sd = vfx_sd(vfx, &color, r);
    //float sd = sd_cube(r, vfx->c*2.0);
    //fprintf(stderr, "%d:%g\n", i, sd);
    if (sd > BASE_DISTANCE*2.0) return 0;
    if (sd < MIN_DIST) {
      //*hit = r;
      //return 0xFF0000;
      for (j = 0; j < 10; j++) {
        color = vfx_get(vfx, (int)r.x, (int)r.y, (int)r.z);
        if (color) break;
        r += rd * 0.1f;
      }
      *hit = r;
      return color; //0xFF0000;
    }
    r += rd*sd;
  }


  return 0;
}

