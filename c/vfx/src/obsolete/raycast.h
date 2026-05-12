
typedef struct {
  float t;
  uint32_t node;
  vec3 o;
} oct_hit_t;

#define S(i,j) do { \
    if (a##i->t > a##j->t) { \
      oct_hit_t *temp = a##i; a##i = a##j; a##j = temp; \
    } \
  } while (0)


INLINE void sort_hits2(oct_hit_t **a) {
    oct_hit_t *a0, *a1;
    a0=a[0];a1=a[1];
    S(0, 1);
    a[0]=a0;a[1]=a1;
}


INLINE void sort_hits3(oct_hit_t **a) {
    oct_hit_t *a0, *a1, *a2;
    a0=a[0];a1=a[1];a2=a[2];

    S(0, 2);  S(0, 1);  S(1, 2);

    a[0]=a0;a[1]=a1;a[2]=a2;
}

INLINE void sort_hits4(oct_hit_t **a) {
    oct_hit_t *a0, *a1, *a2, *a3;
    a0=a[0];a1=a[1];a2=a[2];a3=a[3];

    S(0, 1);  S(2, 3);  S(0, 2);  S(1, 3);  S(1, 2);

    a[0]=a0;a[1]=a1;a[2]=a2;a[3]=a3;
}



INLINE float ot_ray_cube_hit(vec3 rd_inv, vec3 o, vec3 vc) {
  vec3 da = o;
  vec3 db = da + vc;

  //first get the times to corner A planes and corner B planes
  vec3 times_A = vmin(da,db) * rd_inv;
  vec3 times_B = vmax(da,db) * rd_inv;

  //determine the near and far planes
  float tmin = maxe(vmin(times_A,times_B)); //time enter
  float tmax = mine(vmax(times_A,times_B)); //time leave

  if (tmin >= tmax) return INFINITY; //exit before entrance = no intersection
  if (tmin < 0.0f) return INFINITY; //ray origin is inside or past cube

  return tmin;
}

//#define BUG 1


//FIXME: optimize by using global BVH `t`
//FIXME: unrolling this will allow quick return
static void ot_raymarch(rt_t *RESTRICT rt, uint32_t node, vec3 o, vec3 vc) {
  int oct, i;
  ot_t *this = (ot_t*)rt->svo;
  uint32_t *octs = this->topo.ref(node)->octs;
  oct_hit_t hits[4];
  oct_hit_t *phits[4];
  int nhits = 0;
  for(oct = 0; oct < 8; oct++) {
    uint32_t next = octs[oct];
    if (!next) continue; //empty
    vec3 to = o + oct2gt[oct]*vc;
    SVO_PRF(ot_vcnt++);
    float t = ot_ray_cube_hit(rt->ird, to, vc);
    if (!(t < INFINITY)) continue; //miss
    hits[nhits].t = t;
    hits[nhits].o = to;
    hits[nhits].node = next;
    nhits++;
    if (nhits == 4) break; //can never have more than 4 hits
  }
  switch(nhits) {
  case 0: return;
  case 1: phits[0] = &hits[0]; break;
  case 2: 
    phits[0] = &hits[0];
    phits[1] = &hits[1];
    sort_hits2(phits);
    break;
  case 3:
    phits[0] = &hits[0];
    phits[1] = &hits[1];
    phits[2] = &hits[2];
    sort_hits3(phits);
    break;
  case 4:
    phits[0] = &hits[0];
    phits[1] = &hits[1];
    phits[2] = &hits[2];
    phits[3] = &hits[3];
    sort_hits4(phits);
    break;
  //default: fprintf(stderr, "hits overflow: %d\n", nhits); return 0;
  }
  vc *= 0.5;
  for(i = 0; i < nhits; i++) {
    oct_hit_t *ph = phits[i];
    uint32_t node = ph->node;
    if (IS_TERM(node)) {
      rt->t = ph->t;
      rt->attr = this->attr.ref(node);
      return;
    }
    ot_raymarch(rt, OT_FPTR(node), ph->o, vc);
    if (rt->attr) return;
  }
}
#undef S


//special hit variations for the raymarcher, using pre-inverted ray direction
INLINE int ray_aabb_hit_test1(vec3 ro, vec3 rd_inv, vec3 box) {
  vec3 a = -ro*rd_inv;
  vec3 b = (box - ro)*rd_inv;
  float tmin = maxe(vmin(a,b));
  float tmax = mine(vmax(a,b));
  return tmax >= fmax(0.0f, tmin);
}

INLINE void ot_t.raycast(rt_t *RESTRICT rt) {
  SVO_PRF(ot_rcnt++);
  rt->t = INFINITY;
  rt->attr = 0;
  rt->ird = 1.0f/(rt->rd + (vec3){FLT_MIN,FLT_MIN,FLT_MIN});
  if (!ray_aabb_hit_test1(rt->ro, rt->ird, rt->box))
    return; //miss scene box
  float c = (float)this->c;
  vec3 vc = {c,c,c};
  //-ro avoids subing it in the ray_cube intersection code
  ot_raymarch(rt, 0, -rt->ro, vc);
  if (rt->attr) {
    //printf("hello: %x\n", rt->attr->color);exit(-1);
  }
}

