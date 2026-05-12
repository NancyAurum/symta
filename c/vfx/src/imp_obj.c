#include <fast_obj.h>

#:slb
#:mesh

class !fastObjMesh;
class !fastObjGroup;
class !fastObjIndex;

int vxImportObj(slab_t *slb, char *filename) {
  fastObjMesh *om = fast_obj_read(filename);
  //printf("%p: %s\n", om, filename);
  if (!om) return 0;

  auto mesh = new(CMesh);

  auto vertices = &mesh->vertices;
  auto faces = &mesh->faces;

  for (U32 ii = 0; ii < om->group_count; ii++) {
    fastObjGroup *grp = &om->groups[ii];
    //if (grp->name) say((char*)grp->name);
    int idx = 0;
    for (U32 jj = 0; jj < grp->face_count; jj++) {
      U32 fv = om->face_vertices[grp->face_offset + jj];
      int tt = 0;
      for (U32 kk = 0; kk < fv; kk++) {
        fastObjIndex mi = om->indices[grp->index_offset + idx];
        F32 x = om->positions[3 * mi.p + 0];
        F32 y = om->positions[3 * mi.p + 1];
        F32 z = om->positions[3 * mi.p + 2];
        F32 tu = om->texcoords[2 * mi.t + 0];
        F32 tv = om->texcoords[2 * mi.t + 1];
        CVertex v;
        v.xyz = (vec3){x,y,z};
        v.uv = (vec2){tu,tv};
        vertices.push(v);
        tt++;
        if (tt >= 3) {
          CFace face;
          U32 vi = vertices.len;
          face.vs[0] = vi-tt; //faces are in triangle fan format
          face.vs[1] = vi-2;
          face.vs[2] = vi-1;
          faces.push(face);
        }
        idx++;
      }
    }
    break; //FIXME: implement importing a list of objects
  }
  fast_obj_destroy(om);
  mesh.rescale(1.0,MESH_FLIP_X|MESH_POSITIVE);
  vxVoxelize(slb, mesh);
  delete(mesh);
  vxFillInterior(slb);

  return 1;

}

