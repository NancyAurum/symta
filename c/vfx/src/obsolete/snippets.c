
//origin https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
//note: the box is centered at origin.
static float sdBox(vec3 p, vec3 b) {
  vec3 q, qclamp;
  VABS(q, p);
  q -= b;
  vec3 o = {0.0,0.0,0.0};
  VMAXV(qclamp, q, o);
  float qlen = VLEN(qclamp);
  return qlen + MIN(MAX(q.x,MAX(q.y,q.z)),0.0);
  
  /*vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);*/
}

static float sdSphere(vec3 p, float s) {
  return VLEN(p)-s;
}

/*

// r.dir is unit direction vector of ray
dirfrac.x = 1.0f / r.dir.x;
dirfrac.y = 1.0f / r.dir.y;
dirfrac.z = 1.0f / r.dir.z;
// lb is the corner of AABB with minimal coordinates - left bottom, rt is maximal corner
// r.org is origin of ray
float t1 = (lb.x - r.org.x)*dirfrac.x;
float t2 = (rt.x - r.org.x)*dirfrac.x;
float t3 = (lb.y - r.org.y)*dirfrac.y;
float t4 = (rt.y - r.org.y)*dirfrac.y;
float t5 = (lb.z - r.org.z)*dirfrac.z;
float t6 = (rt.z - r.org.z)*dirfrac.z;

float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

// if tmax < 0, ray (line) is intersecting AABB, but the whole AABB is behind us
if (tmax < 0)
{
    t = tmax;
    return false;
}

// if tmin > tmax, ray doesn't intersect AABB
if (tmin > tmax)
{
    t = tmax;
    return false;
}

t = tmin;
return true;

//If this returns true, it's intersecting, if it returns false, it's not intersecting.

If you use the same ray many times, you can precompute dirfrac (only division in whole intersection test). And then it's really fast. And you have also length of ray until intersection (stored in t).

*/


namespace {
    //Helper function for Line/AABB test.  Tests collision on a single dimension
    //Param:    Start of line, Direction/length of line,
    //          Min value of AABB on plane, Max value of AABB on plane
    //          Enter and Exit "timestamps" of intersection (OUT)
    //Return:   True if there is overlap between Line and AABB, False otherwise
    //Note:     Enter and Exit are used for calculations and are only updated in case of intersection
    bool Line_AABB_1d(float start, float dir, float min, float max, float& enter, float& exit)
    {
        //If the line segment is more of a point, just check if it's within the segment
        if(fabs(dir) < 1.0E-8)
            return (start >= min && start <= max);

        //Find if the lines overlap
        float   ooDir = 1.0f / dir;
        float   t0 = (min - start) * ooDir;
        float   t1 = (max - start) * ooDir;

        //Make sure t0 is the "first" of the intersections
        if(t0 > t1)
            Math::Swap(t0, t1);

        //Check if intervals are disjoint
        if(t0 > exit || t1 < enter)
            return false;

        //Reduce interval based on intersection
        if(t0 > enter)
            enter = t0;
        if(t1 < exit)
            exit = t1;

        return true;
    }
}

//Check collision between a line segment and an AABB
//Param:    Start point of line segement, End point of line segment,
//          One corner of AABB, opposite corner of AABB,
//          Location where line hits the AABB (OUT)
//Return:   True if a collision occurs, False otherwise
//Note:     If no collision occurs, OUT param is not reassigned and is not considered useable
bool CollisionDetection::Line_AABB(const Vector3D& s, const Vector3D& e, const Vector3D& min, const Vector3D& max, Vector3D& hitPoint)
{
    float       enter = 0.0f;
    float       exit = 1.0f;
    Vector3D    dir = e - s;

    //Check each dimension of Line/AABB for intersection
    if(!Line_AABB_1d(s.x, dir.x, min.x, max.x, enter, exit))
        return false;
    if(!Line_AABB_1d(s.y, dir.y, min.y, max.y, enter, exit))
        return false;
    if(!Line_AABB_1d(s.z, dir.z, min.z, max.z, enter, exit))
        return false;

    //If there is intersection on all dimensions, report that point
    hitPoint = s + dir * enter;
    return true;
}




// -- I'm used to operator overloading and classes,
// -- so treat this as pseudocode if you don't have that luxury
// -- you might want to expand these anyway for optimization
struct vec3 {
    double x,y,z;
    vec3 operator-(const vec3& other) const {
        return vec3{;
            x - other.x,
            y - other.y,
            z - other.z
        }
    }
    vec3 operator*(double scalar) const {
        return vec3{
            x * scalar,
            y * scalar,
            z * scalar
        };      
    }
    vec3 operator/(const vec3& other) const {
        return vec3{
            x / other.x,
            y / other.y,
            z / other.z
        };  
    }
    double max() const {
        return max(max(x, y), z);
    }
    double min() const {
        return min(min(x, y), z);   
    }
};

int ray_cube_hit(
    vec3 ray_origin,
    vec3 ray_direction,
    vec3 cube_corner_A,
    vec3 cube_corner_B,
    vec3* first_hitpoint_OUTPARAM,
    vec3* second_hitpoint_OUTPARAM
){
    //distances to planes (relative to ray origin)
    vec3 distances_A = cube_corner_A - ray_origin;
    vec3 distances_B = cube_corner_B - ray_origin;

    vec3 distances_min = {
        min(distances_A.x, distances_B.x),
        min(distances_A.y, distances_B.y),
        min(distances_A.z, distances_B.z)
    };
    vec3 distances_max = {
        max(distances_A.x, distances_B.x),
        max(distances_A.y, distances_B.y),
        max(distances_A.z, distances_B.z)
    };

    //times for ray to pass through plane
    vec3 times_min = distances_min / ray_direction;//divide element by element
    vec3 times_max = distances_max / ray_direction;

    //time for passing all three planes once, and leaving the earliest second plane
    double time_enter = times_min.max_element();
    double time_leave = times_max.min_element();

    //hitpoints
    first_hitpoint_OUTPARAM = ray_origin + ray_direction * time_enter;
    second_hitpoint_OUTPARAM = ray_origin + ray_direction * time_leave;

    return 1;
}



static INLINE
int ray_cube_hit2(xyz_t *RESTRICT hita, xyz_t *RESTRICT hitb //hit points
                ,xyz_t *RESTRICT src, xyz_t *RESTRICT dir //ray origin/direction
                ,double sx, double sy, double sz  //min x,y,z
                ,double ex, double ey, double ez  //max x,y,z
          ) {
  xyz_t cube_corner_A, cube_corner_B, distances_A, distances_B,
        distances_min, distances_max, times_min, times_max,
        first_hitpoint_OUTPARAM, second_hitpoint_OUTPARAM;
  VSET(cube_corner_A, sx,sy,sz);
  VMOV(distances_A, cube_corner_A);
  VSUB(distances_A, *src);
  VMOV(distances_B, cube_corner_B);
  VSUB(distances_B, *src);

  distances_min.x = MIN(distances_A.x,distances_B.x);
  distances_min.y = MIN(distances_A.y,distances_B.y);
  distances_min.z = MIN(distances_A.z,distances_B.z);

  distances_max.x = MAX(distances_A.x,distances_B.x);
  distances_max.y = MAX(distances_A.y,distances_B.y);
  distances_max.z = MAX(distances_A.z,distances_B.z);

#define VDIVE(a,b) \
   do { (a).x /= (b).x; (a).y /= (b).y; (a).z /= (b).z; } while(0)

  VMOV(times_min, distances_min);
  VDIVE(times_min, *dir);
  VMOV(times_max, distances_max);
  VDIVE(times_max, *dir);

#define VMIN(v) MIN((v).x,MIN((v).y,(v).z))
#define VMAX(v) MAX((v).x,MAX((v).y,(v).z))
  double time_enter = VMAX(times_min);
  double time_leave = VMIN(times_max);
  VMOV(first_hitpoint_OUTPARAM,*dir);
  VMUL(first_hitpoint_OUTPARAM,time_enter);
  VADD(first_hitpoint_OUTPARAM,*src);
  VMOV(second_hitpoint_OUTPARAM,*dir);
  VMUL(second_hitpoint_OUTPARAM,time_leave);
  VADD(second_hitpoint_OUTPARAM,*src);
  VMOV(*hita, first_hitpoint_OUTPARAM);
  VMOV(*hitb, second_hitpoint_OUTPARAM);

  return 1;
}


/*
the Graphics Gems algorithm is simply:

- Using your ray's direction vector, determine which 3 of the 6 candidate planes would be hit first. If your (unnormalized) ray direction vector is (-1, 1, -1), then the 3 planes that are possible to be hit are +x, -y, and +z.

- Of the 3 candidate planes, do find the t-value for the intersection for each. Accept the plane that gets the largest t value as being the plane that got hit, and check that the hit is within the box. The diagram in the text makes this clear:
*/


#define NUMDIM  3

#define RIGHT   0
#define LEFT    1
#define MIDDLE  2

#define TRUE    1
#define FALSE   0


static INLINE
int HitBoundingBox(double minB[NUMDIM],double maxB[NUMDIM] //box
                  ,double origin[NUMDIM],double dir[NUMDIM] //ray
                  ,double coord[NUMDIM]) //hit point
{
	char inside = TRUE;
	char quadrant[NUMDIM];
	int i;
	int whichPlane;
	double maxT[NUMDIM];
	double candidatePlane[NUMDIM];

	// Find candidate planes; this loop can be avoided if
  // rays cast all from the eye(assume perpsective view)
	for (i=0; i<NUMDIM; i++)
		if (origin[i] < minB[i]) {
			quadrant[i] = LEFT;
			candidatePlane[i] = minB[i];
			inside = FALSE;
		} else if (origin[i] > maxB[i]) {
			quadrant[i] = RIGHT;
			candidatePlane[i] = maxB[i];
			inside = FALSE;
		} else	{
			quadrant[i] = MIDDLE;
		}

	// Ray origin inside bounding box
	if (inside)	{
		coord = origin;
		return (TRUE);
	}

	// Calculate T distances to candidate planes 
	for (i = 0; i < NUMDIM; i++)
		if (quadrant[i] != MIDDLE && dir[i] != 0.0)
			maxT[i] = (candidatePlane[i]-origin[i]) / dir[i];
		else
			maxT[i] = -1.0;

	// Get largest of the maxT's for final choice of intersection
	whichPlane = 0;
	for (i = 1; i < NUMDIM; i++)
		if (maxT[whichPlane] < maxT[i])
			whichPlane = i;

	// Check final candidate actually inside box
	if (maxT[whichPlane] < 0.0) return (FALSE);
	for (i = 0; i < NUMDIM; i++)
		if (whichPlane != i) {
			coord[i] = origin[i] + maxT[whichPlane] * dir[i];
			if (coord[i] < minB[i] || coord[i] > maxB[i])
				return (FALSE);
		} else {
			coord[i] = candidatePlane[i];
		}
	return (TRUE);	 // ray hits box
}

#define X 0
#define Y 1
#define Z 2

static INLINE
int ray_cube_hit_h(xyz_t *RESTRICT hit //hit point
                ,xyz_t *RESTRICT src, xyz_t *RESTRICT dir //ray origin/direction
                ,double sx, double sy, double sz  //min x,y,z
                ,double ex, double ey, double ez  //max x,y,z
          ) {
  double minB[NUMDIM];
  double maxB[NUMDIM];
  double origin[NUMDIM];
  double rdir[NUMDIM];
  double coord[NUMDIM];
  minB[X] = sx;
  minB[Y] = sy;
  minB[Z] = sz;
  maxB[X] = ex;
  maxB[Y] = ey;
  maxB[Z] = ez;
  origin[X] = src->x;
  origin[Y] = src->y;
  origin[Z] = src->z;
  rdir[X] = dir->x;
  rdir[Y] = dir->y;
  rdir[Z] = dir->z;
  int result = HitBoundingBox(minB,maxB,origin,rdir,coord);
  if (!result) return 0;
  hit->x = coord[X];
  hit->y = coord[Y];
  hit->z = coord[Z];
  return 1;
}

static INLINE
int ray_cube_hit(xyz_t *RESTRICT hita, xyz_t *RESTRICT hitb //hit points
                ,xyz_t *RESTRICT src, xyz_t *RESTRICT dir //ray origin/direction
                ,double sx, double sy, double sz  //min x,y,z
                ,double ex, double ey, double ez  //max x,y,z
          ) {
  double c = ex-sx;
  if (!ray_cube_hit_h(hita, src,  dir, sx, sy, sz, ex, ey, ez)) return 0;

  xyz_t rdir;
  VNEG(rdir,*dir);
  xyz_t rsrc;
  VMOV(rsrc,*dir);
  VMUL(rsrc,c*1000);
  VADD(rsrc,*src);
  if (!ray_cube_hit_h(hitb, &rsrc, &rdir, sx, sy, sz, ex, ey, ez)) return 0;
  return 1;
}




float rayBoxIntersect ( float3 rpos, float3 rdir, float3 vmin, float3 vmax )
{
   float t[10];
   t[1] = (vmin.x - rpos.x)/rdir.x;
   t[2] = (vmax.x - rpos.x)/rdir.x;
   t[3] = (vmin.y - rpos.y)/rdir.y;
   t[4] = (vmax.y - rpos.y)/rdir.y;
   t[5] = (vmin.z - rpos.z)/rdir.z;
   t[6] = (vmax.z - rpos.z)/rdir.z;
   t[7] = fmax(fmax(fmin(t[1], t[2]), fmin(t[3], t[4])), fmin(t[5], t[6]));
   t[8] = fmin(fmin(fmax(t[1], t[2]), fmax(t[3], t[4])), fmax(t[5], t[6]));
   t[9] = (t[8] < 0 || t[7] > t[8]) ? NOHIT : t[7];
   return t[9];
}


bool intersectRayAABox2(const Ray &ray, const Box &box, int& tnear, int& tfar)
{
    Vector3d T_1, T_2; // vectors to hold the T-values for every direction
    double t_near = -DBL_MAX; // maximums defined in float.h
    double t_far = DBL_MAX;

    for (int i = 0; i < 3; i++){ //we test slabs in every direction
        if (ray.direction[i] == 0){ // ray parallel to planes in this direction
            if ((ray.origin[i] < box.min[i]) || (ray.origin[i] > box.max[i])) {
                return false; // parallel AND outside box : no intersection possible
            }
        } else { // ray not parallel to planes in this direction
            T_1[i] = (box.min[i] - ray.origin[i]) / ray.direction[i];
            T_2[i] = (box.max[i] - ray.origin[i]) / ray.direction[i];

            if(T_1[i] > T_2[i]){ // we want T_1 to hold values for intersection with near plane
                swap(T_1,T_2);
            }
            if (T_1[i] > t_near){
                t_near = T_1[i];
            }
            if (T_2[i] < t_far){
                t_far = T_2[i];
            }
            if( (t_near > t_far) || (t_far < 0) ){
                return false;
            }
        }
    }
    tnear = t_near; tfar = t_far; // put return values in place
    return true; // if we made it here, there was an intersection - YAY
}

bool AABB::intersects( const Ray& ray )
{
  // EZ cases: if the ray starts inside the box, or ends inside
  // the box, then it definitely hits the box.
  // I'm using this code for ray tracing with an octree,
  // so I needed rays that start and end within an
  // octree node to COUNT as hits.
  // You could modify this test to (ray starts inside and ends outside)
  // to qualify as a hit if you wanted to NOT count totally internal rays
  if( containsIn( ray.startPos ) || containsIn( ray.getEndPoint() ) )
    return true ; 

  // the algorithm says, find 3 t's,
  Vector t ;

  // LARGEST t is the only one we need to test if it's on the face.
  for( int i = 0 ; i < 3 ; i++ )
  {
    if( ray.direction.e[i] > 0 ) // CULL BACK FACE
      t.e[i] = ( min.e[i] - ray.startPos.e[i] ) / ray.direction.e[i] ;
    else
      t.e[i] = ( max.e[i] - ray.startPos.e[i] ) / ray.direction.e[i] ;
  }

  int mi = t.maxIndex() ;
  if( BetweenIn( t.e[mi], 0, ray.length ) )
  {
    Vector pt = ray.at( t.e[mi] ) ;

    // check it's in the box in other 2 dimensions
    int o1 = ( mi + 1 ) % 3 ; // i=0: o1=1, o2=2, i=1: o1=2,o2=0 etc.
    int o2 = ( mi + 2 ) % 3 ;

    return BetweenIn( pt.e[o1], min.e[o1], max.e[o1] ) &&
           BetweenIn( pt.e[o2], min.e[o2], max.e[o2] ) ;
  }

  return false ; // the ray did not hit the box.
}


static inline double dmin(double a, double b) {
  return a < b ? a : b;
}

static inline double dmax(double a, double b) {
  return a > b ? a : b;
}

typedef struct vector_t {
  double n[3];
} vector_t;

static inline vector_t new_vector(double x, double y, double z) {
  vector_t v = { { x, y, z } };
  return v;
}


typedef struct ray_t {
  vector_t x0; /**< The origin of the ray. */
  vector_t n;  /**< The direction of the ray. */
} ray_t;

/// A ray with pre-calculated reciprocals to avoid divisions.
typedef struct optimized_ray_t {
  vector_t x0;    ///< The origin of the ray.
  vector_t n_inv; ///< The inverse of each component of the ray's slope
} optimized_ray_t;

/** An axis-aligned bounding box. */
typedef struct box_t {
  vector_t min; /**< The coordinate-wise minimum extent of the box. */
  vector_t max; /**< The coordinate-wise maximum extent of the box. */
} box_t;

#define X n[0]
#define Y n[1]
#define Z n[2]

/// Precompute inverses for faster ray-box intersection tests.
static inline optimized_ray_t optimize_ray(ray_t ray) {
  optimized_ray_t optray = {
    .x0    = ray.x0,
    .n_inv = new_vector(1.0/ray.n.X, 1.0/ray.n.Y, 1.0/ray.n.Z)
  };
  return optray;
}


/// Ray-AABB intersection test, by the slab method.
static inline int ray_cube_hit_f(optimized_ray_t optray, box_t box, double t) {
  // This is actually correct, even though it appears not to handle edge cases
  // (ray.n.{x,y,z} == 0).  It works because the infinities that result from
  // dividing by zero will still behave correctly in the comparisons.  Rays
  // which are parallel to an axis and outside the box will have tmin == inf
  // or tmax == -inf, while rays inside the box will have tmin and tmax
  // unchanged.

  double tx1 = (box.min.X - optray.x0.X)*optray.n_inv.X;
  double tx2 = (box.max.X - optray.x0.X)*optray.n_inv.X;

  double tmin = dmin(tx1, tx2);
  double tmax = dmax(tx1, tx2);

  double ty1 = (box.min.Y - optray.x0.Y)*optray.n_inv.Y;
  double ty2 = (box.max.Y - optray.x0.Y)*optray.n_inv.Y;

  tmin = dmax(tmin, dmin(ty1, ty2));
  tmax = dmin(tmax, dmax(ty1, ty2));

  double tz1 = (box.min.Z - optray.x0.Z)*optray.n_inv.Z;
  double tz2 = (box.max.Z - optray.x0.Z)*optray.n_inv.Z;

  tmin = dmax(tmin, dmin(tz1, tz2));
  tmax = dmin(tmax, dmax(tz1, tz2));

  return tmax >= dmax(0.0, tmin) && tmin < t;
}


static INLINE
int ray_cube_hit_test(xyz_t *RESTRICT hita, xyz_t *RESTRICT hitb
                ,xyz_t *RESTRICT src, xyz_t *RESTRICT dir
                ,double sx, double sy, double sz
                ,double ex, double ey, double ez) {
  ray_t ray;
  optimized_ray_t optray;
  box_t box;
  ray.x0.X = src->x;
  ray.x0.Y = src->y;
  ray.x0.Z = src->z;
  ray.n.X = dir->x;
  ray.n.Y = dir->y;
  ray.n.Z = dir->z;
  box.min.X = sx;
  box.min.Y = sy;
  box.min.Z = sz;
  box.max.X = ex;
  box.max.Y = ey;
  box.max.Z = ez;
  optray = optimize_ray(ray);
  return ray_cube_hit_f(optray, box, 0.5);
}
