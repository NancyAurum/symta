#:slb
#:camera

void vxCamera(float ox, float oy, float oz  //camera origin
             ,float xx, float xy, float xz  //camera right direction
             ,float yx, float yy, float yz  //camera upward direction
             ,float zx, float zy, float zz  //camera font direction
             ,float scale                   //scale of result projection
             ,float focal_length
             ,U32 screen_w
             ,U32 screen_h
             ,U32 flags
) {
  gcam.ro = (vec3){ox, oy, oz};
  gcam.xb = (vec3){xx, xy, xz};
  gcam.yb = (vec3){yx, yy, yz};
  gcam.zb = (vec3){zx, zy, zz};
  gcam.scale = scale;
  if (flags & CAM_PERSPECTIVE) gcam.scale *= focal_length;
  gcam.focal_length = 1;
  gcam.screen_w = screen_w;
  gcam.screen_h = screen_h;
  gcam.flags = flags;
}

void vxOrient(slab_t *slb, U32 flags
               ,float sx, float sy, float sz
               ,float cx, float cy, float cz
               ,float ax, float ay, float az
               ,float x, float y, float z
               ,float r, float g, float b

) {
  slb->flags = flags;
  //say("hello: ", x,",",y,",",z); //this crashed the NCC compiler in the past
  slb->scale = (vec3){sx,sy,sz};
  slb->center = (vec3){cx,cy,cz};
  slb->angle = (vec3){ax,ay,az};
  slb->xyz = (vec3){x,y,z};
  slb->color = (vec3){r,g,b};
}

