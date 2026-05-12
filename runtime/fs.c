#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ng.h"
#include "fs.h"


#define SAF_IMPLEMENTATION
#include "../saf/saf.h"


//mount system remaps and redirects paths
typedef struct {
  int sl;
  char *src;
  char *dst;
  uint32_t flags;
} fs_mount_t;

static fs_mount_t **mounts = 0;

void fs_mount(char *src, char *dst, uint32_t flags) {
  fs_mount_t *m = malloc(sizeof(fs_mount_t));
  m->sl = strlen(src);
  m->src = strdup(src);
  m->dst = strdup(dst);
  m->flags = flags;
  arrput(mounts, m);
}

static int fs_ready = 0;

char *fs_resolve(char **saf_name, char *path) {
  if (!fs_ready) {
#if 0
    fs_mount("saf:"
            , "/Users/macbook/Documents/git/symta/saf/dst.saf"
            ,MNT_SAF);
#endif
    fs_ready = 1;
  }
  char *r = 0;
again:
  for (int i = 0; i < arrlen(mounts); i++) {
    fs_mount_t *m = mounts[i];
    if (!strncmp(m->src, path, m->sl)) {
      if (m->flags&MNT_SAF) {
        *saf_name = m->dst;
        path = strdup(path+m->sl);
        if (r) free(r);
        r = path;
        goto again;
      } else {
        path = cat(m->dst,path+m->sl);
        if (r) free(r);
        r = path;
        goto again;
      }
    }
  }
  return r;
}

int fs_is_folder(char *path) {
  char *saf_name = 0;
  char *rpath = fs_resolve(&saf_name, path);
  
  if (rpath) path = rpath;
  int r = 0;

  if (saf_name) r = saf_is_folder(saf_name, path);
  else r = path_is_folder(path);
  if (rpath) free(rpath);
  return r;
}


int fs_is_file(char *path) {
  char *saf_name = 0;
  char *rpath = fs_resolve(&saf_name, path);
  
  if (rpath) path = rpath;

  int r = 0;
  if (saf_name) r = saf_is_file(saf_name, path);
  else r = path_is_file(path);

  if (rpath) free(rpath);

  return r;
}

uint64_t fs_time(char *path) {
  char *saf_name = 0;
  char *rpath = fs_resolve(&saf_name, path);
  
  if (rpath) path = rpath;
  uint64_t r = 0;

  if (saf_name) {
    r = file_time(saf_name); //all files inside SAF have the SAF time.
  } else {
    r = file_time(path);
  }
  if (rpath) free(rpath);
  return r;
}

int fs_exists(char *path) {
  char *saf_name = 0;
  char *rpath = fs_resolve(&saf_name, path);
  
  if (rpath) path = rpath;
  int r = 0;

  if (saf_name) {
    r = (saf_is_file(saf_name, path) || saf_is_folder(saf_name, path));
  } else {
    r = file_exists(path);
  }
  if (rpath) free(rpath);
  return r;
}

uint8_t *fs_get(int64_t *size, char *path) {
  char *saf_name = 0;
  char *rpath = fs_resolve(&saf_name, path);
  
  if (rpath) path = rpath;

  int64_t fsz;
  uint8_t *data;

  if (saf_name) {
    data = saf_get(&fsz, saf_name, path);
  } else if (path_is_folder(path)) {
    char **es = list_folder(path, 0);
    char *t = 0;
    for (int i = 0; i < arrlen(es); i++) {
      for (char *q = es[i]; *q; q++) arrput(t, *q);
      arrput(t, '\n');
      free(es[i]);
    }
    arrfree(es);
    if (!t) {
      data = strdup("");
      fsz = 0;
    } else {
      data = malloc(arrlen(t));
      memcpy(data, t, arrlen(t));
      fsz = arrlen(t);
    }
  } else {
    data = read_whole_file(path, &fsz);
  }
  if (rpath) free(rpath);
  
  if (size) *size = fsz;

  return data;
}

int fs_copy(char *src, char *dst) {
  int64_t size;
  uint8_t *data = fs_get(&size, src);
  if (!data) return 0;
  write_whole_file(dst, data, (int)size);
  free(data);
  return 1;
}
