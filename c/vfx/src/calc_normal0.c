vec3 calc_normal0(slab_t *slb, ivec3 p) {
ivec3 dir;
ivec3 q = p-(ivec3){1,1,1};
ivec3 r = p+(ivec3){1,1,1};
int w = slb->w, h = slb->h, d = slb->d;
ivec3 n = {0,0,0};
if ((U32)q.x<w&&(U32)q.y<h&&(U32)q.z<d&&
    (U32)r.x<w&&(U32)r.y<h&&(U32)r.z<d) {
dir = (ivec3){-1,-1,-1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){0,-1,-1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){1,-1,-1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){-1,0,-1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){0,0,-1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){1,0,-1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){-1,1,-1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){0,1,-1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){1,1,-1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){-1,-1,0};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){0,-1,0};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){1,-1,0};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){-1,0,0};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){1,0,0};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){-1,1,0};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){0,1,0};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){1,1,0};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){-1,-1,1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){0,-1,1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){1,-1,1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){-1,0,1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){0,0,1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){1,0,1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){-1,1,1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){0,1,1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
dir = (ivec3){1,1,1};
q = p+dir;
if (slb.svo.nil(q.x,q.y,q.z)) n += dir;
} else {
dir = (ivec3){-1,-1,-1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){0,-1,-1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){1,-1,-1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){-1,0,-1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){0,0,-1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){1,0,-1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){-1,1,-1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){0,1,-1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){1,1,-1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){-1,-1,0};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){0,-1,0};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){1,-1,0};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){-1,0,0};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){1,0,0};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){-1,1,0};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){0,1,0};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){1,1,0};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){-1,-1,1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){0,-1,1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){1,-1,1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){-1,0,1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){0,0,1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){1,0,1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){-1,1,1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){0,1,1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
dir = (ivec3){1,1,1};
q = p+dir;
if ((U32)q.x>=w||(U32)q.y>=h||(U32)q.z>=d
   || slb.svo.nil(q.x,q.y,q.z)) {
  n += dir;
}
}
return normalized((vec3){n.x,n.y,n.z});
}
