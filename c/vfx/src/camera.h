#camera_setup() {
camera_t cam = gcam;
float cam_scale = cam.scale*(float)(cam.screen_w+cam.screen_h)/2.0;

int is_perspective = cam.flags & CAM_PERSPECTIVE;

vec3 sxb = cam.xb;   //screen x-basis
vec3 syb = cam.yb;   //screen y-basis
vec3 szb = cam.zb;   //screen z-basis
vec3 wrot = (vec3){0.0,0.0,0.0}; //FIXME: deduce this from basis
//say("o=", cam.ro,"xb=",sxb,"; yb=",syb,"; zb=",szb);


float scale = cam_scale;

//if (is_perspective) scale *= 2.10;

float scaleI = 1.0/scale;
sxb *= scaleI;
syb *= scaleI;

int sw = cam.screen_w; //screen width and height
int sh = cam.screen_h;
vec3 sxp; //x-coord in the projected screen
vec3 syp;


vec3 focal;
if (is_perspective) {
  float fl = cam.focal_length;
  focal = cam.ro;
  syp = focal + szb*fl;
  //focal = syp - szb*fl + (float)sh*0.5f*syb - (float)sw*0.5f*sxb;
} else {
  syp = cam.ro; //y-coord in the projected screen
}

vec3 screen_center = syp;

syp -= sxb * (float)sw*0.5f;
syp += syb * (float)sh*0.5f;

//shot rays from the centers of pixels
syp -= sxb*0.5;
syp += syb*0.5;

}

#RAYDIR(p) (is_perspective ? normalized((p)-focal) : szb)
