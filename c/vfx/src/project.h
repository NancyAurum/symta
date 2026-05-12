
#project_setup() {
//remove slb->xyz from the equation asap
gcam.ro -= slb->xyz;
camera_setup()
gcam.ro += slb->xyz;

sxp = syp;

scene_item_t si = slab_to_scene_item(slb);
scene_item_t *RESTRICT item = &si;
//FIXME: note we don't add item->center here to save computations
vec3 rom0 = vmm(screen_center,item->view); //screen center inside item's coords
vec3 rom = rom0 + slb->center;
//vec3 rdm = normalized((vmm(szb*PRECSCL+sxp,item->view) - rom0)/PRECSCL);
//vec3 rdm = normalized(vmm(szb+screen_center,item->view) - rom0);
vec3 rdm = normalized(vmm(szb,item->view));
vec3 screen_dir = -rdm;
vec3 focalm = rom - rdm/item->scale;

mat3 rmm = item->rview; //reverse model view

mat3 mw;
mw.X = cam.xb;
mw.Y = cam.yb;
mw.Z = szb;
//should sxb and syb be used instead?
mw.X *= -cam.scale*cam.screen_w;
mw.Y *=  cam.scale*cam.screen_h;

ot_t *svo = slb.svo;
}

#project_setupG() {
  int gw = gfx->w;
  int gh = gfx->h;
  int gw1 = gw-1;
  int gh1 = gh-1;
  float gw05 = gw*0.5f;
  float gh05 = gh*0.5f;
  project_setup();
  U32 *pixels = gfx->data;
  float aspect = sqrt((float)gw/gh); //why sqrt?
}

#PROJECT_HEADER(CONTINUE_STMNT) {
  F32 odist = ray_plane_hit_dist(p, -rdm, rom, rdm);
  if (odist < 0.0f) CONTINUE_STMNT;
  if (is_perspective) {
    screen_dir = normalized(focalm-p);
  }

  //get distance from point to the screen plane
  F32 dist = ray_plane_hit_dist(p, screen_dir, rom, rdm);

  vec3 hit = (screen_dir * dist) + p;

  //reverse the model rotation around center
  vec3 hitw = vmm(hit-slb->center, rmm);

  // project onto screen
  vec3 scp = vmm(hitw-screen_center, mw);
  
  //float angle = dot(normalized(hit-p),screen_dir);
}

#project_body(CONTINUE_STMNT) {
  PROJECT_HEADER(CONTINUE_STMNT);

  if (dist < 0.0f) CONTINUE_STMNT;

  int scx = (int)(gw05 - scp.x/aspect);
  if (scx < 0) {
    if (scx < -1) CONTINUE_STMNT;
    scx = 0;
  }
  if (scx > gw1) {
    if (scx > gw) CONTINUE_STMNT;
    scx = gw1;
  }
  int scy = (int)(gh05 - scp.y*aspect);
  if (scy < 0) {
    if (scy < -1) CONTINUE_STMNT;
    scy = 0;
  }
  if (scy > gh1) {
    if (scy > gh) CONTINUE_STMNT;
    scy = gh1;
  }

  uint32_t pixel = pixels[scy*gw+scx];
  uint32_t transparent = pixel&0xFF000000;
}


#PROJECT_POINT(CONTINUE_STMNT, out, px,py,pz) { {
  vec3 p = {px,py,pz};
  PROJECT_HEADER(CONTINUE_STMNT);
  out.x = -scp.x;
  out.y = -scp.y;
} }


inline void cube_bounds(vec2 *ps, vec2 *minO, vec2 *maxO) {
  float minX = INFINITY, minY = INFINITY;
  float maxX = -INFINITY, maxY = -INFINITY;
  for (int i = 0; i < 8; i++) {
    minX = fmin(minX, ps[i].x);
    minY = fmin(minY, ps[i].y);
    maxX = fmax(maxX, ps[i].x);
    maxY = fmax(maxY, ps[i].y);
  }
  minO.x = minX;
  minO.y = minY;
  maxO.x = maxX;
  maxO.y = maxY;
}

#PROJECT_CUBE(CONTINUE_STMNT) {
  vec2 ps[8], minP, maxP;
  int project_cube_miss = 0;
  PROJECT_POINT(project_cube_miss++, ps[0], sx  ,sy  ,sz  );
  PROJECT_POINT(project_cube_miss++, ps[1], sx+c,sy  ,sz  );
  PROJECT_POINT(project_cube_miss++, ps[2], sx  ,sy+c,sz  );
  PROJECT_POINT(project_cube_miss++, ps[3], sx+c,sy+c,sz  );
  PROJECT_POINT(project_cube_miss++, ps[4], sx  ,sy  ,sz+c);
  PROJECT_POINT(project_cube_miss++, ps[5], sx+c,sy  ,sz+c);
  PROJECT_POINT(project_cube_miss++, ps[6], sx  ,sy+c,sz+c);
  PROJECT_POINT(project_cube_miss++, ps[7], sx+c,sy+c,sz+c);
  if (project_cube_miss>6) CONTINUE_STMNT;
  cube_bounds(ps, &minP, &maxP);
  int bx = fmax(roundf(minP.x+gw05),0);
  int by = fmax(roundf(minP.y+gh05),0);
  int ex = fmin(roundf(maxP.x+gw05),gw1);
  int ey = fmin(roundf(maxP.y+gh05),gh1);
}
