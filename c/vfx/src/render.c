#:slb
#:camera


typedef struct {
  vec3 xyz;
  vec3 angle;
  vec3 intensity; //intensity
} light_t;

TLst(LightsList,light_t)

static LightsList scene_lights;

TLst(SceneItems,scene_item_t)
static SceneItems scene_list;

TLst(ScenePItems,scene_item_t*)

typedef struct {
  aabb_t aabb[2]; //AABB for each kid
  int kids[2];
} bvh_node_t;

TLst(BVHList,bvh_node_t)
static BVHList bvh;

//FIXME: a items by their last time render hits
static int build_bvh_r(ScenePItems *RESTRICT items) {
  int i, j;

  scene_item_t **elts = items.elts;
  int node_index = (int)bvh.len;
  bvh_node_t *node = bvh.add;

  if (items.len == 2) {
    node->kids[0] = elts[0]->index;
    node->aabb[0] = elts[0]->aabb;
    node->kids[1] = elts[1]->index;
    node->aabb[1] = elts[1]->aabb;
    return -node_index;
  }
  vec3 bmin = (vec3){INFINITY,INFINITY,INFINITY};
  vec3 bmax = (vec3){-INFINITY,-INFINITY,-INFINITY};

  for (j = 0; j < items.len; j++) {
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
    for (j = 0; j < items.len; j++) {
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
  spl[0].init(items.len);
  spl[1].init(items.len);
  if (best_dist > -INFINITY) {
    for (j = 0; j < items.len; j++) {
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
    int pivot = items.len/2;
    for (j = 0; j < items.len; j++) {
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
    assert(spl[i].len != 0);
    if (spl[i].len == 1) {
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
  items.init(scene_list.len);
  for (int j = 0; j < scene_list.len; j++) {
    scene_item_t *RESTRICT item = &scene_elts[j];
    item->index = j;
    items.push(item);
  }
  bvh.free;
  bvh.init(MAX(scene_list.len*2,10));
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

scene_item_t slab_to_scene_item(slab_t *slb) {
  scene_item_t si;
  si.slb = slb;
  si.view = mrotated(midentity(),vdeg2rad(slb->angle));
  si.scale = slb->scale;
  si.view = mscaled(si.view,1.0f/si.scale);
  si.o = slb->center + vmm(slb->xyz, si.view);

  mat3 idm = mscaled(midentity(),si.scale);
  si.rview = mrotated_r(idm,vdeg2rad(slb->angle));
  return si;
}

void vxSlab(slab_t *slb) {
  if (slb->flags&SLB_LIGHT) {
    light_t light;
    light.xyz = slb->xyz;
    light.angle = slb->angle;
    light.intensity = slb->color;
    scene_lights.push(light);
  }
  slb->xyz = -slb->xyz; //FIXME: kludge
  if (!(slb->flags&SLB_INVISIBLE)) {
    scene_list.push(slab_to_scene_item(slb));
  }
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


#SHD_FLAT               0x0
#SHD_POINTS             0x1
#SHD_CUBES              0x2
#SHD_ZBUFFER            0x3
#SHD_NBUFFER            0x4
#SHD_TEST               0x5
#SHD_CUBIC_POINTS       0x6

#SHD_SMOOTH       0x0100
#SHD_LOWFI        0x0200
#SHD_GLOW         0x0400
#SHD_PILLOW       0x0800
#SHD_ZCUE         0x1000
#SHD_NEEDS_PREV   0x2000
#SHD_SKYBOX_BG    0x4000

#SHD_TYPE(flags)  ((flags)&0xFF)
#SHD_FLAGS(flags)  ((flags)&0xFFFFFF00)

static U32 shd_type;  //shader type
static U32 shd_flags;
typedef U32 (*shd_fn_t)(shd_t *RESTRICT s);
typedef U32 (*shd_sfn_t)(shd_t *RESTRICT s, vec3 ro, vec3 rd);
class !shd_fn_t; //hack
class !shd_sfn_t; //hack
static shd_fn_t shd_fn;
static shd_sfn_t shd_sky;
static U32 shd_sky_color;
static vec3 shd_ambient; //ambient light

#SHD_VARS U32 color=s->voxel;float t=s->t; vec3 p=s->p;vec3 n=s->n; 

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
  for (int i = 0; i < scene_lights.len; i++) {
    light_t *RESTRICT light = &lights[i];
    vec3 ldir = light->xyz - p;
    //angle should not be negative, otherwise we will get shadows
    //reducing ambient light.
    float angle = fmax(0.0f, vangle(ldir,n));
    l += angle*light->intensity;
  }
  U32 r,g,b;
  UNRGB(r,g,b,color);
  U32 rl = ((U32)((F32)r*l.x)).clamp_byte;
  U32 gl = ((U32)((F32)g*l.y)).clamp_byte;
  U32 bl = ((U32)((F32)b*l.z)).clamp_byte;
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
  return s->voxel&0xFFFFFF;
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

static U32 shd_sky_solid(shd_t *RESTRICT s, vec3 ro, vec3 rd) {
  return shd_sky_color;
}

//sky + earth skybox algorithm taken from NVidia's demo
static U32 shd_sky_skybox(shd_t *RESTRICT s, vec3 ro, vec3 rd) {
  vec3 c;
  vec3 L = {0.3643f, 0.3535f, 0.8616f};
  vec3 I = rd;
  if (I.y >= 0.0f) { //sky?
    vec3 horz = { 179.0f, 205.0f, 253.0f };
    vec3 zen  = { 77.0f,  102.0f, 179.0f };
    c = horz + (zen - horz) * I.y * I.y;
    c *= 2.5f;
  } else { //earth
    vec3 horz = { 192.0f, 154.0f, 102.0f };
    vec3 zen  = { 128.0f, 102.0f, 77.0f };
    c = horz - (zen - horz) * I.y;
    //return shd_sky_color;
  }

  c *= fmaxf(L.y, 0.0f);
  float IL = dot(I, L);
  if (IL > 0.0f) {
    c += (vec3){255.0f, 179.0f, 102.0f} * powf(IL, 1000.0f); // sun
  }

  int r = ((int)c.x).clamp_byte;
  int g = ((int)c.y).clamp_byte;
  int b = ((int)c.z).clamp_byte;
  return RGB(r, g, b);
}

static U32 shd_sky_skybox_sp(shd_t *RESTRICT s, vec3 ro, vec3 rd) {
  vec3 inf = (vec3){INFINITY,INFINITY,INFINITY};
  s->prev_col = inf;
  s->prev_row[s->sx] = inf;
  return shd_sky_skybox(s, ro, rd);
}

static U32 shd_sky_solid_sp(shd_t *RESTRICT s, vec3 ro, vec3 rd) {
  vec3 inf = (vec3){INFINITY,INFINITY,INFINITY};
  s->prev_col = inf;
  s->prev_row[s->sx] = inf;
  if (!s->sx || !s->sy) return 0x7f7f7f;
  return shd_sky_color;
}


void vxShader(U32 flags) {
  shd_type = SHD_TYPE(flags);
  shd_flags = flags;
  int skybox_bg = flags & SHD_SKYBOX_BG;
  shd_fn = &shd_points;
  shd_sky = skybox_bg ? &shd_sky_skybox : &shd_sky_solid;
  if (shd_type == SHD_NBUFFER) shd_fn = &shd_nbuffer;
  else if (shd_type == SHD_ZBUFFER) shd_fn = &shd_zbuffer;
  else if (shd_type == SHD_FLAT) shd_fn = &shd_flat;
  else if (shd_type == SHD_CUBES) {
    shd_fn = &shd_cubes;
    shd_flags |= SHD_NEEDS_PREV;
    shd_sky = skybox_bg ? &shd_sky_skybox_sp : &shd_sky_solid_sp;
  } else if (shd_type == SHD_CUBIC_POINTS) {
    shd_fn = &shd_cubic_points;
    shd_flags |= SHD_NEEDS_PREV;
    shd_sky = skybox_bg ? &shd_sky_skybox_sp : &shd_sky_solid_sp;
  }
}

static void generate_aabbs() {
  scene_item_t *RESTRICT scene_elts = scene_list.elts;
  
  for (int j = 0; j < scene_list.len; j++) {
    scene_item_t *RESTRICT item = &scene_elts[j];
    slab_t *slb = item->slb;
    mat3 idm = mscaled(midentity(),item->scale);
    mat3 mm = mrotated_r(idm,vdeg2rad(slb->angle));

    vec3 bmin = (vec3){INFINITY,INFINITY,INFINITY};
    vec3 bmax = (vec3){-INFINITY,-INFINITY,-INFINITY};

    for (int i = 0; i < 8; i++) {
      vec3 corner = oct2gt[i];
      vec3 p = corner*slb->box;
      //reverse the model rotation around center
      vec3 pw = vmm(p-slb->center, mm)-slb->xyz;
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
  while (scene_list.len < 2) vxSlab(dummy_slab);
  generate_aabbs();
  build_bvh();
}



#if 0
//FIXME: should we raymarch to detemine if the point is actually seen
//       nearest voxels should be excluded from blocking the ray?
inline vec3 calc_normalN(slab_t *slb, ivec3 p, int soft) {
  int c = soft; //smooth factor
  int x,y,z;
  ivec3 n = {0,0,0};
  float count = 0.0f;
  int w = slb->w;
  int h = slb->h;
  int d = slb->d;
  ot_t *svo = slb.svo;
  for (z = -c; z <= c; z++) { //cast rays into all directions
    for (y = -c; y <= c; y++) {
      for (x = -c; x <= c; x++) {
        if (x == 0 && y == 0 && z == 0) continue;
        ivec3 dir = (ivec3){x,y,z};
        ivec3 q = p+dir;
        if ((U32)q.x>=w || (U32)q.y>=h || (U32)q.z>=d
            || svo.nil(q.x,q.y,q.z)) {
          n += dir;
          count++;
        }
      }
    }
  }
  return normalized((vec3){n.x,n.y,n.z});
}
#else
//FIXME: raymarch to detemine if the point is actually seen
//       nearest voxels should be excluded from blocking the ray.
inline vec3 calc_normalN(slab_t *slb, ivec3 p, int soft) {
  int c = soft; //smooth factor
  int x,y,z;
  ivec3 n = {0,0,0};
  int w = slb->w;
  int h = slb->h;
  int d = slb->d;
  int px = p.x, py = p.y, pz = p.z;
  ot_t *svo = slb.svo;
  for (z = -c; z <= c; z++) { //cast rays into all directions
    int zz = pz+z;
    if ((U32)zz >= d) for (y = -c; y <= c; y++) {
      for (x = -c; x <= c; x++) n += (ivec3){x,y,z};
    } else for (y = -c; y <= c; y++) {
      int yy = py+y;
      if ((U32)yy >= h) {
        for (x = -c; x <= c; x++) n += (ivec3){x,y,z};
      } else {
        if (y == 0 && z == 0) {
          for (x = -c; x <= c; x++) {
            if (x == 0) continue;
            int xx = px+x;
            if ((U32)xx >= w || svo.nil(xx,yy,zz)) n += (ivec3){x,y,z};
          }
        } else {
          if (   (px>>5) == ((px+c)>>5) && (px>>5) == ((px-c)>>5)
              && (py>>5) == ((py+c)>>5) && (py>>5) == ((py-c)>>5)
              && (pz>>5) == ((pz+c)>>5) && (pz>>5) == ((pz-c)>>5)
              && svo->c >= 32) {
            //all inside the same segment
            seg_t *seg = svo.seg(px,py,pz);
            int yyy = yy&0x1F;
            int zzz = zz&0x1F;
            int xxx = (px - c)&0x1F;
#if 0
            switch(c) {
#:calc_normal_unroll.c
            }
#else
            for (x = -c; x <= c; x++, xxx++) {
              if (seg.nil16(xxx,yyy,zzz)) n += (ivec3){x,y,z};
            }
#endif
          } else {
            for (x = -c; x <= c; x++) {
              int xx = px+x;
              if ((U32)xx >= w || svo.nil(xx,yy,zz)) n += (ivec3){x,y,z};
            }
          }
        }
      }
    }
  }
  return normalized((vec3){n.x,n.y,n.z});
}
#endif

#:calc_normal0.c

attr_t *vxRecalcNormal(slab_t *slb, vec3 p, vec3 *rn, U32 soft) {
  //generate_normal_sum_loop(); exit(-1);
  U32 x = p.x;
  if (UNLIKELY(x >= (U32)slb->w)) goto ret_dummy;
  U32 y = p.y;
  if (UNLIKELY(y >= (U32)slb->h)) goto ret_dummy;
  U32 z = p.z;
  if (UNLIKELY(z >= (U32)slb->d)) goto ret_dummy;
  ivec3 xyz = (ivec3){x,y,z};
#if 1
  vec3 n;
  switch(soft) {
  case 0: n = calc_normal0(slb, xyz); break;
  default: n = calc_normalN(slb, xyz, soft+1);
  }
#else
  vec3 n = calc_normalN(slb, xyz, soft+1);
#endif
  if (rn) *rn = n;
  attr_t *grain = slb.svo.set_existing(x,y,z);
  //attr_t *grain = 0;
  if (!grain) { //FIXME: hack to prevent setting non-existing voxels
ret_dummy:;
    static attr_t dummy_attr;
    return &dummy_attr;
  }
  grain->normal = vnormal2rgb(n);
  return grain;
}

void ray_bvh_hit(int n, shd_t *RESTRICT hit, vec3 ro, vec3 rd) {
  bvh_node_t *node = bvh.ref(-n);

  int order[2];
  float ts[2];
  ts[0] = ray_box_hit_test(ro, rd, node->aabb[0].min, node->aabb[0].max).x;
  ts[1] = ray_box_hit_test(ro, rd, node->aabb[1].min, node->aabb[1].max).x;
  if (ts[0] > ts[1]) {
    float t = ts[0];
    ts[0] = ts[1];
    ts[1] = t;
    order[0] = 1;
    order[1] = 0;
  } else {
    order[0] = 0;
    order[1] = 1;
  }

  if (!(ts[0] < INFINITY)) return; //both ray-box tests are misses.

  for (int i = 0; i < 2; i++) {
    int o = order[i];
    int kid = node->kids[o];
    if (kid<0) { //non-leaf?
      ray_bvh_hit(kid, hit, ro, rd);
      goto next;
    }
    scene_item_t *RESTRICT item = &scene_list.elts[kid];
    //FIXME: rd*PRECSCL and ro/PRECSCL is a hack to avoid precision loss in rd
    vec3 rom = vmm(ro,item->view) + item->o;
    vec3 rdm = vmm(rd*PRECSCL,item->view)/PRECSCL;
    rt_t rtt, *rt = &rtt;
    rt.init(item->slb);
    rt->ro = rom;
    rt->rd = rdm;
    rt.shot;
    float t = rt->t;
    if (t < hit->t) {
      hit->t = t;
      attr_t attr = rt->attr;
      hit->voxel = attr.color;
      slab_t *slb = item->slb;
      hit->slb = slb;
      //vec3 p = rdm*(rt->t+0.0001f) + rom; //FIXME: 0.0001 is to get inside voxel
      vec3 p = rdm*(rt->t+0.0005f) + rom; //FIXME: 0.0001 is to get inside voxel
      vec3 n;
      U32 n32 = attr.normal;
      if (n32 >= MESH_NORMAL && !attr.is_empty) {
        //p = vround(p);
        U32 soft = (attr.color&SOFTNESS_MASK)>>24;
        attr_t *grain = vxRecalcNormal(slb,p,&n,soft);
      } else {
        n = vrgb2normal(attr.normal);
      }

      //vec3 nm = ray_plane_hit(p+n, -rdm, rom, rdm); //project onto screen
      hit->n = vmm(n,item->rview); //direction can be just rotated
      /*vec3 nm = rom+n;
      vec3 nw = vmm(nm-slb->center, item->rview)-slb->xyz; //to world
      hit->n = nw-ro;*/

    }
  next:
    if (hit->t <= ts[1]) return; //hit before the second box.
    if (ts[1] == INFINITY) return; //missed the second box
  }
}


U32 vx_render_cycle = 0;

//FIXME: consider sorting items by the distance towards screen.
void vxRender(gfx_t *cbuf) {
  camera_setup();

  vx_render_cycle++;

  SVO_PRF(ot_rcnt=0); SVO_PRF(ot_vcnt=0);
#if #PROFILE
  double TStart = nano_clock();
#endif

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
      vec3 rd = RAYDIR(sxp);
      ray_bvh_hit(0, &s, sxp, rd);
      if (s.t < INFINITY) {
        s.p = sxp + s.t*rd;
        *pcs = shd_fn(&s);
      } else {
        *pcs = shd_sky(&s, sxp, rd);
      }
      pcs++;
      sxp += sxb;
      bug = 0;
    }
    syp -= syb;
  }
  if (shd_flags&SHD_NEEDS_PREV) {
    free(s.prev_row);
  }
  SVO_PRF(fprintf(stderr, "vxRender: rays=%d, voxels=%d vpr=%g\n"
                , rcnt, vcnt, (float)vcnt/(float)rcnt));

#if #PROFILE
  double TEnd = nano_clock();
  double TPassed = TEnd-TStart; 
  fprintf(stderr, "vxRender time: %g seconds\n", TPassed);
#endif
}

char *vxSampleRay(slab_t *slb, U32 scx, U32 scy) {
  gcam.ro -= slb->xyz;
  camera_setup();

  sxp = syp;
  sxp += sxb*(float)scx;
  sxp -= syb*(float)scy;

  scene_item_t si = slab_to_scene_item(slb);
  scene_item_t *RESTRICT item = &si;

  vec3 ro = sxp;
  vec3 rd = RAYDIR(sxp);

  vec3 rom = vmm(ro,item->view) + slb->center;
  vec3 rdm = vmm(rd*PRECSCL,item->view)/PRECSCL;
  rt_t rtt, *rt = &rtt;
  rt.init(item->slb);
  rt->ro = rom;
  rt->rd = rdm;
  rt.shot;
  if (rt->t == INFINITY) {
    sprintf(slb_sample_txt, "void");
    return slb_sample_txt;
  }
  vec3 hit = rdm*(rt->t+0.0005f) + rom;
  sprintf(slb_sample_txt, "%f %f %f 0x%x",hit.x,hit.y,hit.z
         ,rt->attr.color&0xFFFFFF);
  return slb_sample_txt;
}


U32 vxRenderSample(int scx, int scy) {
  camera_setup();

  sxp = syp;
  sxp += sxb*(float)scx;
  sxp -= syb*(float)scy;

  shd_t s;
  s.sw = sw;
  s.sh = sh;

  if (shd_flags&SHD_NEEDS_PREV) {
    s.prev_row = malloc(sizeof(vec3)*4*1024);
  }

  U32 sample;

  s.t = INFINITY;
  vec3 rd = RAYDIR(sxp);
  ray_bvh_hit(0, &s, sxp, rd);
  if (s.t < INFINITY) {
    s.p = sxp + s.t*rd;
    sample = shd_fn(&s);
  } else {
    sample = NIL_COLOR; //shd_sky(&s);
  }

  if (shd_flags&SHD_NEEDS_PREV) {
    free(s.prev_row);
  }
  return sample;
}

#:project

char slb_sample_txt[1024];

char *vxSampleAABB(slab_t *slb) {
  slb->xyz = -slb->xyz;
  project_setup();

  int gw = cam.screen_w;
  int gh = cam.screen_h;
  int gw1 = gw-1;
  int gh1 = gh-1;
  float gw05 = gw*0.5f;
  float gh05 = gh*0.5f;

  char cs[8][64];

  for (int i = 0; i < 8; i++) {
    vec3 corner = oct2gt[i];
    vec3 p = corner*slb->box;

    PROJECT_HEADER( );

    vec3 sp = -scp;
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
