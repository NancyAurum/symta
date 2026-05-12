//Origin: http://paulbourke.net/geometry/polygonise/

#include <ncu/util.h>

#:slb
#:mesh


/* 
	Simple test of the Marching Cubes code found in paulslib found here
	Very poorly written with lots of assumptions, designed to give the
   basic idea of how to call PolygoniseCube().
	ps: code formateed for tab stops of 3 characters.
	pps: One would normally want to calculate normals as well.
*/

typedef struct {
   double x,y,z;
} XYZ;

typedef struct {
   XYZ p[8];
   XYZ n[8];
   double val[8];
} GRIDCELL;

typedef struct {
   XYZ p[3];         /* Vertices */
   XYZ c;            /* Centroid */
   XYZ n[3];         /* Normal   */
} TRIANGLE;

#ABS(x) (x < 0 ? -(x) : (x))

// Prototypes
int PolygoniseCube(GRIDCELL,double,TRIANGLE *);
XYZ VertexInterp(double,XYZ,XYZ,double,double);


#COLOR(x,y,z) data[(z)*w*h + (y)*w + (x)]

uint32_t *vxPlainArray(slab_t *slb, int margin) {
  int i, j;
  int w=slb->w, h=slb->h, d=slb->d;
  w += margin*2;
  h += margin*2;
  d += margin*2;
	uint32_t *data = malloc(w*h*d*sizeof(uint32_t));
  for (i = 0; i<w*h*d; i++) {
    data[i] = NIL_COLOR;
  }
  segs_t *segs = slb.svo.list_segs;
  for (int j = 0; j < segs.len; j++) {
    seg_t *seg = segs.get(j);
    SVOTerms *terms = seg.filled_terms;
    for (int i = 0; i < terms.len; i++) {
      term_t *t = terms.ref(i);
      U32 x = t.x;
      U32 y = t.y;
      U32 z = t.z;
      if (!(x < slb->w && y < slb->h && z < slb->d)) {
        continue;
      }
      
      if (t->c != 1) {
        seg.term_split(t,terms);
        continue;
      }

      attr_t *attr = seg.term_attr(t);
      x += margin;
      y += margin;
      z += margin;
      COLOR(x,y,z) = attr->color&0xFFFFFF;
    }
    delete(terms);
    seg.compact;
  }
  delete(segs);

  return data;
}



CMesh *vxPolygonize(slab_t *slb) {
  int w=slb->w+2, h=slb->h+2, d=slb->d+2;
	uint32_t *data = vxPlainArray(slb, 1);

  auto mesh = new(CMesh);

  auto vertices = &mesh->vertices;
  auto faces = &mesh->faces;

  //first just count the number of visible colors
  int ncolors = 0;
	for (int z = 0; z < d; z++) {
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
        uint32_t color = COLOR(x,y,z);
        if (color == NIL_COLOR) continue;
        if (   COLOR(x-1,y,z) != NIL_COLOR
            && COLOR(x+1,y,z) != NIL_COLOR
            && COLOR(x,y-1,z) != NIL_COLOR
            && COLOR(x,y+1,z) != NIL_COLOR
            && COLOR(x,y,z-1) != NIL_COLOR
            && COLOR(x,y,z+1) != NIL_COLOR)
        {
          continue; //completely occluded
        }
        ncolors++;
      }
    }
  }
  ncolors *= 4;

  //texture to store colors
  int cw = MESH_TEXELS_PER_LINE;
  int ch = (ncolors+MESH_TEXELS_PER_LINE-1)/MESH_TEXELS_PER_LINE;
  auto tx = vxMeshNewTexture(cw, ch);
  tx.clear(0);
  mesh->colors = tx;

  float dx = 1.0f/cw;
  float dy = 1.0f/ch;

  //FIXME: kludge to defeat texture filtering. We sample only near center.
  float cdx = 0.4999f/cw;
  float cdy = 0.4999f/ch;

  int color_index = 0;
	for (int x=0; x < w-1; x++) {
		for (int y=0; y < h-1; y++) {
			for (int z=0; z < d-1; z++) {
        TRIANGLE tris[10];
        GRIDCELL grid;
				grid.p[0].x = x;
				grid.p[0].y = y;
        grid.p[0].z = z;
				grid.val[0] = COLOR(x,y,z);
        grid.p[1].x = x+1;
        grid.p[1].y = y;
        grid.p[1].z = z; 
				grid.val[1] = COLOR(x+1,y,z);
        grid.p[2].x = x+1;
        grid.p[2].y = y+1;
        grid.p[2].z = z;
				grid.val[2] = COLOR(x+1,y+1,z);
        grid.p[3].x = x;
        grid.p[3].y = y+1;
        grid.p[3].z = z;
				grid.val[3] = COLOR(x,y+1,z);
        grid.p[4].x = x;
        grid.p[4].y = y;
        grid.p[4].z = z+1;
				grid.val[4] = COLOR(x,y,z+1);
        grid.p[5].x = x+1;
        grid.p[5].y = y;
        grid.p[5].z = z+1;
				grid.val[5] = COLOR(x+1,y,z+1);
        grid.p[6].x = x+1;
        grid.p[6].y = y+1;
        grid.p[6].z = z+1;
				grid.val[6] = COLOR(x+1,y+1,z+1);
        grid.p[7].x = x;
        grid.p[7].y = y+1;
        grid.p[7].z = z+1;
				grid.val[7] = COLOR(x,y+1,z+1);
				int ntris = PolygoniseCube(grid,128,tris);
				for (int i=0; i < ntris; i++) {
          CFace face;
          U32 vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          vec3 a = (vec3){tris[i].p[0].x, tris[i].p[0].y, tris[i].p[0].z};
          vec3 b = (vec3){tris[i].p[1].x, tris[i].p[1].y, tris[i].p[1].z};
          vec3 c = (vec3){tris[i].p[2].x, tris[i].p[2].y, tris[i].p[2].z};
          vec3 center = vround((a+b+c)/3.0f);
          int xx = fmax(1, fmin(w-2,center.x));
          int yy = fmax(1, fmin(h-2,center.y));
          int zz = fmax(1, fmin(d-2,center.z));
          uint32_t color = COLOR(xx,yy,zz);

          int cx = color_index%MESH_TEXELS_PER_LINE;
          int cy = color_index/MESH_TEXELS_PER_LINE;
          if (cx < cw && cy < ch) {
            tx->data[cw*cy + cx] = color;
          }
          color_index++;
          cy = ch - cy;
          vec2 uv = (vec2){(float)cx/cw + cdx, (float)cy/ch + cdy};
  
          for (int j=0; j<3; j++)  {
            float x = tris[i].p[j].x;
            float y = tris[i].p[j].y;
            float z = tris[i].p[j].z;
            CVertex v;
            v.xyz = (vec3){x,y,z};
            v.uv = uv;
            vertices.push(v);
          }
        }
			}
		}
	}

  free(data);
  mesh.rescale(1.0, MESH_FLIP_X);
  mesh->flags |= MESH_TEXTURED;

  return mesh;
}


CMesh *vxPolygonizeExact(slab_t *slb) {
  int w=slb->w+2, h=slb->h+2, d=slb->d+2;

	int i,j,x,y,z;

	uint32_t *data = vxPlainArray(slb, 1);

  auto mesh = new(CMesh);

  auto vertices = &mesh->vertices;
  auto faces = &mesh->faces;

  //first just count the number of visible colors
  int ncolors = 0;
	for (z = 0; z < d; z++) {
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
        uint32_t color = COLOR(x,y,z);
        if (color == NIL_COLOR) continue;
        if (   COLOR(x-1,y,z) != NIL_COLOR
            && COLOR(x+1,y,z) != NIL_COLOR
            && COLOR(x,y-1,z) != NIL_COLOR
            && COLOR(x,y+1,z) != NIL_COLOR
            && COLOR(x,y,z-1) != NIL_COLOR
            && COLOR(x,y,z+1) != NIL_COLOR)
        {
          continue; //completely occluded
        }
        ncolors++;
      }
    }
  }

  //texture to store colors
  int cw = MESH_TEXELS_PER_LINE;
  int ch = (ncolors+MESH_TEXELS_PER_LINE-1)/MESH_TEXELS_PER_LINE;
  auto tx = vxMeshNewTexture(cw, ch);
  tx.clear(0);
  mesh->colors = tx;

  float dx = 1.0f/cw;
  float dy = 1.0f/ch;

  //FIXME: kludge to defeat texture filtering. We sample only near center.
  float cdx = 0.4999f/cw;
  float cdy = 0.4999f/ch;

  int color_index = 0;
	for (z = 0; z < d; z++) {
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
        uint32_t color = COLOR(x,y,z);
        if (color == NIL_COLOR) continue;
        if (   COLOR(x-1,y,z) != NIL_COLOR
            && COLOR(x+1,y,z) != NIL_COLOR
            && COLOR(x,y-1,z) != NIL_COLOR
            && COLOR(x,y+1,z) != NIL_COLOR
            && COLOR(x,y,z-1) != NIL_COLOR
            && COLOR(x,y,z+1) != NIL_COLOR)
        {
          continue; //completely occluded
        }

        float fx = x;
        float fy = y;
        float fz = z;
        CVertex v;
        CFace face;
        U32 vi;

        int cx = color_index%MESH_TEXELS_PER_LINE;
        int cy = color_index/MESH_TEXELS_PER_LINE;

        tx->data[cw*cy + cx] = color;
        color_index++;

        cy = ch - cy;

        float tx = (float)cx/cw;
        float ty = (float)cy/ch;
        vec2 uva = (vec2){tx+cdx   ,ty-cdy   };
        vec2 uvb = (vec2){tx-cdx+dx,ty-cdy   };
        vec2 uvc = (vec2){tx+cdx   ,ty+cdy-dy};
        vec2 uvd = (vec2){tx-cdx+dx,ty+cdy-dy};

        if (COLOR(x,y,z-1) == NIL_COLOR) {
          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Front Top Left:
          v.xyz = (vec3){fx-0.5f,fy+0.5f,fz-0.5f};
          v.uv = uva;
          vertices.push(v);

          //Front Top Right:
          v.xyz = (vec3){fx+0.5f,fy+0.5f,fz-0.5f};
          v.uv = uvb;
          vertices.push(v);

          //Front Bottom Left:
          v.xyz = (vec3){fx-0.5f,fy-0.5f,fz-0.5f};
          v.uv = uvc;
          vertices.push(v);

          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Front Bottom Left:
          v.xyz = (vec3){fx-0.5f,fy-0.5f,fz-0.5f};
          v.uv = uvc;
          vertices.push(v);

          //Front Bottom Right:
          v.xyz = (vec3){fx+0.5f,fy-0.5f,fz-0.5f};
          v.uv = uvd;
          vertices.push(v);

          //Front Top Right:
          v.xyz = (vec3){fx+0.5f,fy+0.5f,fz-0.5f};
          v.uv = uvb;
          vertices.push(v);
        }

        if (COLOR(x,y,z+1) == NIL_COLOR) {
          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Back Top Left:
          v.xyz = (vec3){fx-0.5f,fy+0.5f,fz+0.5f};
          v.uv = uva;
          vertices.push(v);

          //Back Top Right:
          v.xyz = (vec3){fx+0.5f,fy+0.5f,fz+0.5f};
          v.uv = uvb;
          vertices.push(v);

          //Back Bottom Left:
          v.xyz = (vec3){fx-0.5f,fy-0.5f,fz+0.5f};
          v.uv = uvc;
          vertices.push(v);

          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Back Bottom Left:
          v.xyz = (vec3){fx-0.5f,fy-0.5f,fz+0.5f};
          v.uv = uvc;
          vertices.push(v);

          //Back Bottom Right:
          v.xyz = (vec3){fx+0.5f,fy-0.5f,fz+0.5f};
          v.uv = uvd;
          vertices.push(v);

          //Back Top Right:
          v.xyz = (vec3){fx+0.5f,fy+0.5f,fz+0.5f};
          v.uv = uvb;
          vertices.push(v);
        }

        if (COLOR(x-1,y,z) == NIL_COLOR) {
          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Left Top Left:
          v.xyz = (vec3){fx-0.5f,fy+0.5f,fz+0.5f};
          v.uv = uva;
          vertices.push(v);

          //Left Top Right:
          v.xyz = (vec3){fx-0.5f,fy+0.5f,fz-0.5f};
          v.uv = uvb;
          vertices.push(v);

          //Left Bottom Left:
          v.xyz = (vec3){fx-0.5f,fy-0.5f,fz+0.5f};
          v.uv = uvc;
          vertices.push(v);

          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Left Bottom Left:
          v.xyz = (vec3){fx-0.5f,fy-0.5f,fz+0.5f};
          v.uv = uvc;
          vertices.push(v);

          //Left Bottom Right:
          v.xyz = (vec3){fx-0.5f,fy-0.5f,fz-0.5f};
          v.uv = uvd;
          vertices.push(v);

          //Left Top Right:
          v.xyz = (vec3){fx-0.5f,fy+0.5f,fz-0.5f};
          v.uv = uvb;
          vertices.push(v);
        }

        if (COLOR(x+1,y,z) == NIL_COLOR) {
          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Right Top Left:
          v.xyz = (vec3){fx+0.5f,fy+0.5f,fz+0.5f};
          v.uv = uva;
          vertices.push(v);

          //Right Top Right:
          v.xyz = (vec3){fx+0.5f,fy+0.5f,fz-0.5f};
          v.uv = uvb;
          vertices.push(v);

          //Right Bottom Left:
          v.xyz = (vec3){fx+0.5f,fy-0.5f,fz+0.5f};
          v.uv = uvc;
          vertices.push(v);

          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Right Bottom Left:
          v.xyz = (vec3){fx+0.5f,fy-0.5f,fz+0.5f};
          v.uv = uvc;
          vertices.push(v);

          //Right Bottom Right:
          v.xyz = (vec3){fx+0.5f,fy-0.5f,fz-0.5f};
          v.uv = uvd;
          vertices.push(v);

          //Right Top Right:
          v.xyz = (vec3){fx+0.5f,fy+0.5f,fz-0.5f};
          v.uv = uvb;
          vertices.push(v);
        }

        if (COLOR(x,y+1,z) == NIL_COLOR) {
          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Top Top Left:
          v.xyz = (vec3){fx-0.5f,fy+0.5f,fz+0.5f};
          v.uv = uva;
          vertices.push(v);

          //Top Top Right:
          v.xyz = (vec3){fx+0.5f,fy+0.5f,fz+0.5f};
          v.uv = uvb;
          vertices.push(v);

          //Top Bottom Left:
          v.xyz = (vec3){fx-0.5f,fy+0.5f,fz-0.5f};
          v.uv = uvc;
          vertices.push(v);

          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Top Bottom Left:
          v.xyz = (vec3){fx-0.5f,fy+0.5f,fz-0.5f};
          v.uv = uvc;
          vertices.push(v);

          //Top Bottom Right:
          v.xyz = (vec3){fx+0.5f,fy+0.5f,fz-0.5f};
          v.uv = uvd;
          vertices.push(v);

          //Top Top Right:
          v.xyz = (vec3){fx+0.5f,fy+0.5f,fz+0.5f};
          v.uv = uvb;
          vertices.push(v);
        }

        if (COLOR(x,y-1,z) == NIL_COLOR) {
          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Bottom Top Left:
          v.xyz = (vec3){fx-0.5f,fy-0.5f,fz+0.5f};
          v.uv = uva;
          vertices.push(v);

          //Bottom Top Right:
          v.xyz = (vec3){fx+0.5f,fy-0.5f,fz+0.5f};
          v.uv = uvb;
          vertices.push(v);

          //Bottom Bottom Left:
          v.xyz = (vec3){fx-0.5f,fy-0.5f,fz-0.5f};
          v.uv = uvc;
          vertices.push(v);

          vi = vertices.len;
          face.vs[0] = vi;
          face.vs[1] = vi+1;
          face.vs[2] = vi+2;
          faces.push(face);

          //Bottom Bottom Left:
          v.xyz = (vec3){fx-0.5f,fy-0.5f,fz-0.5f};
          v.uv = uvc;
          vertices.push(v);

          //Bottom Bottom Right:
          v.xyz = (vec3){fx+0.5f,fy-0.5f,fz-0.5f};
          v.uv = uvd;
          vertices.push(v);

          //Bottom Top Right:
          v.xyz = (vec3){fx+0.5f,fy-0.5f,fz+0.5f};
          v.uv = uvb;
          vertices.push(v);
        }
			}
		}
	}

  free(data);

  mesh.rescale(1.0, MESH_FLIP_X);
  mesh->flags |= MESH_TEXTURED;

  return mesh;
}




/*-------------------------------------------------------------------------
   Given a grid cell and an isolevel, calculate the triangular
   facets requied to represent the isosurface through the cell.
   Return the number of triangular facets, the array "triangles"
   will be loaded up with the vertices at most 5 triangular facets.
   0 will be returned if the grid cell is either totally above
   of totally below the isolevel.
*/


/*
https://www.reddit.com/r/VoxelGameDev/comments/2ny8ys/need_help_understanding_marching_cubes_isolevel/
ISO Level is an arbitrary constant that signifies that a voxel value is either
inside or outside the surface.

Resolution is how densely packed your points are. For marching cubes you might
go from 1 meter cubes to .5 meter cubes, or to 2 meter cubes which gives a
2x increase in resolution, but 8x increase in data, or .5x resolution and
1/8th the data respectively.

If you're trying to do different resolutions of chunks next to each other
you're going to have to deal with crack-patching between different resolutions.
This is something that is shown in the Transvoxel algo, and a couple others.

You get the differing resolutions by sampling the voxels at different strides.
For instance, if you had a 16x16x16 volume and you wanted the finest grain
sampling you'd sample all of the voxels. If you wanted half the resulotion your
stride would be 2, and you'd only sample every other voxel.
*/

int PolygoniseCube(GRIDCELL g,double iso,TRIANGLE *tri)
{
   int i,ntri = 0;
   int cubeindex;
   XYZ vertlist[12];
/*
   int edgeTable[256].  It corresponds to the 2^8 possible combinations of
   of the eight (n) vertices either existing inside or outside (2^n) of the
   surface.  A vertex is inside of a surface if the value at that vertex is
   less than that of the surface you are scanning for.  The table index is
   constructed bitwise with bit 0 corresponding to vertex 0, bit 1 to vert
   1.. bit 7 to vert 7.  The value in the table tells you which edges of
   the table are intersected by the surface.  Once again bit 0 corresponds
   to edge 0 and so on, up to edge 12.
   Constructing the table simply consisted of having a program run thru
   the 256 cases and setting the edge bit if the vertices at either end of
   the edge had different values (one is inside while the other is out).
   The purpose of the table is to speed up the scanning process.  Only the
   edges whose bit's are set contain vertices of the surface.
   Vertex 0 is on the bottom face, back edge, left side.
   The progression of vertices is clockwise around the bottom face
   and then clockwise around the top face of the cube.  Edge 0 goes from
   vertex 0 to vertex 1, Edge 1 is from 2->3 and so on around clockwise to
   vertex 0 again. Then Edge 4 to 7 make up the top face, 4->5, 5->6, 6->7
   and 7->4.  Edge 8 thru 11 are the vertical edges from vert 0->4, 1->5,
   2->6, and 3->7.
       4--------5     *---4----*
      /|       /|    /|       /|
     / |      / |   7 |      5 |
    /  |     /  |  /  8     /  9
   7--------6   | *----6---*   |
   |   |    |   | |   |    |   |
   |   0----|---1 |   *---0|---*
   |  /     |  /  11 /     10 /
   | /      | /   | 3      | 1
   |/       |/    |/       |/
   3--------2     *---2----*
*/
int edgeTable[256]={
0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0   };

/*
   int triTable[256][16] also corresponds to the 256 possible combinations
   of vertices.
   The [16] dimension of the table is again the list of edges of the cube
   which are intersected by the surface.  This time however, the edges are
   enumerated in the order of the vertices making up the triangle mesh of
   the surface.  Each edge contains one vertex that is on the surface.
   Each triple of edges listed in the table contains the vertices of one
   triangle on the mesh.  The are 16 entries because it has been shown that
   there are at most 5 triangles in a cube and each "edge triple" list is
   terminated with the value -1.
   For example triTable[3] contains
   {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
   This corresponds to the case of a cube whose vertex 0 and 1 are inside
   of the surface and the rest of the verts are outside (00000001 bitwise
   OR'ed with 00000010 makes 00000011 == 3).  Therefore, this cube is
   intersected by the surface roughly in the form of a plane which cuts
   edges 8,9,1 and 3.  This quadrilateral can be constructed from two
   triangles: one which is made of the intersection vertices found on edges
   1,8, and 3; the other is formed from the vertices on edges 9,8, and 1.
   Remember, each intersected edge contains only one surface vertex.  The
   vertex triples are listed in counter clockwise order for proper facing.
*/
int triTable[256][16] =
{{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

   /*
      Determine the index into the edge table which
      tells us which vertices are inside of the surface
   */
   cubeindex = 0;
#if 1
   //voxels are a bit simplier than the scalar field
   if (g.val[0] != NIL_COLOR) cubeindex |= 1;
   if (g.val[1] != NIL_COLOR) cubeindex |= 2;
   if (g.val[2] != NIL_COLOR) cubeindex |= 4;
   if (g.val[3] != NIL_COLOR) cubeindex |= 8;
   if (g.val[4] != NIL_COLOR) cubeindex |= 16;
   if (g.val[5] != NIL_COLOR) cubeindex |= 32;
   if (g.val[6] != NIL_COLOR) cubeindex |= 64;
   if (g.val[7] != NIL_COLOR) cubeindex |= 128;
#else
   if (g.val[0] < iso) cubeindex |= 1;
   if (g.val[1] < iso) cubeindex |= 2;
   if (g.val[2] < iso) cubeindex |= 4;
   if (g.val[3] < iso) cubeindex |= 8;
   if (g.val[4] < iso) cubeindex |= 16;
   if (g.val[5] < iso) cubeindex |= 32;
   if (g.val[6] < iso) cubeindex |= 64;
   if (g.val[7] < iso) cubeindex |= 128;
#endif
   /* Cube is entirely in/out of the surface */
   if (edgeTable[cubeindex] == 0)
      return(0);

   /* Find the vertices where the surface intersects the cube */
   if (edgeTable[cubeindex] & 1) {
      vertlist[0] = VertexInterp(iso,g.p[0],g.p[1],g.val[0],g.val[1]);
   }
   if (edgeTable[cubeindex] & 2) {
      vertlist[1] = VertexInterp(iso,g.p[1],g.p[2],g.val[1],g.val[2]);
   }
   if (edgeTable[cubeindex] & 4) {
      vertlist[2] = VertexInterp(iso,g.p[2],g.p[3],g.val[2],g.val[3]);
   }
   if (edgeTable[cubeindex] & 8) {
      vertlist[3] = VertexInterp(iso,g.p[3],g.p[0],g.val[3],g.val[0]);
   }
   if (edgeTable[cubeindex] & 16) {
      vertlist[4] = VertexInterp(iso,g.p[4],g.p[5],g.val[4],g.val[5]);
   }
   if (edgeTable[cubeindex] & 32) {
      vertlist[5] = VertexInterp(iso,g.p[5],g.p[6],g.val[5],g.val[6]);
   }
   if (edgeTable[cubeindex] & 64) {
      vertlist[6] = VertexInterp(iso,g.p[6],g.p[7],g.val[6],g.val[7]);
   }
   if (edgeTable[cubeindex] & 128) {
      vertlist[7] = VertexInterp(iso,g.p[7],g.p[4],g.val[7],g.val[4]);
   }
   if (edgeTable[cubeindex] & 256) {
      vertlist[8] = VertexInterp(iso,g.p[0],g.p[4],g.val[0],g.val[4]);
   }
   if (edgeTable[cubeindex] & 512) {
      vertlist[9] = VertexInterp(iso,g.p[1],g.p[5],g.val[1],g.val[5]);
   }
   if (edgeTable[cubeindex] & 1024) {
      vertlist[10] = VertexInterp(iso,g.p[2],g.p[6],g.val[2],g.val[6]);
   }
   if (edgeTable[cubeindex] & 2048) {
      vertlist[11] = VertexInterp(iso,g.p[3],g.p[7],g.val[3],g.val[7]);
   }

   /* Create the triangles */
   for (i=0;triTable[cubeindex][i]!=-1;i+=3) {
      tri[ntri].p[0] = vertlist[triTable[cubeindex][i  ]];
      tri[ntri].p[1] = vertlist[triTable[cubeindex][i+1]];
      tri[ntri].p[2] = vertlist[triTable[cubeindex][i+2]];
      ntri++;
   }

   return(ntri);
}

// Return the point between two points in the same ratio as
// isolevel is between valp1 and valp2
XYZ VertexInterp(double isolevel,XYZ p1,XYZ p2,double valp1,double valp2) {
  double mu;
  XYZ p;
  if (ABS(isolevel-valp1) < 0.00001) return(p1);
  if (ABS(isolevel-valp2) < 0.00001) return(p2);
  if (ABS(valp1-valp2) < 0.00001) return(p1);
  mu = (isolevel - valp1) / (valp2 - valp1);
  p.x = p1.x + mu * (p2.x - p1.x);
  p.y = p1.y + mu * (p2.y - p1.y);
  p.z = p1.z + mu * (p2.z - p1.z);
  return(p);
}

