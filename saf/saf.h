#ifndef SAF_H
#define SAF_H
#include <stdint.h>

int saf_create(char *saf_name, char *path);

int saf_extract(char *saf_name, char *path);

//Get file data. The actual bytes are zero terminated for use as a string
uint8_t *saf_get(int64_t *fsize, char *saf_path, char *path);

int saf_is_folder(char *saf_path, char *path);

int saf_is_file(char *saf_path, char *path);

#endif

#ifdef SAF_IMPLEMENTATION

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "ng.h"


#define DEFLATE_C
//#define ULZ_C

#ifdef DEFLATE_C

#include "deflate.c"

static uint8_t *compress(uint32_t *osize, uint8_t *data, uint32_t size) {
  uint32_t cflags = 9;
  uint32_t bsize = deflate_bounds(size, cflags);
  uint8_t *cdata = malloc(bsize);
  uint32_t csize = deflate_encode(data, size, cdata, bsize, cflags);
  //fprintf(stderr, "%d=%d\n",bsize, csize);
  if (osize) *osize = csize;
  return cdata;
}

static uint8_t *decompress(uint8_t *cdata, uint32_t csize, uint32_t size) {
  uint8_t *data = malloc(size+1);
  deflate_decode(cdata, csize, data, size);
  data[size] = 0; //makes text files easier to process
  return data;
}

#elif defined(ULZ_C)
#include "ulz.c"

static uint8_t *compress(uint32_t *osize, uint8_t *data, uint32_t size) {
  uint32_t cflags = 9;
  uint32_t bsize = ulz_bounds(size, cflags);
  uint8_t *cdata = malloc(bsize);
  uint32_t csize = ulz_encode(data, size, cdata, bsize, cflags);
  //fprintf(stderr, "%d=%d\n",bsize, csize);
  if (osize) *osize = csize;
  return cdata;
}


static uint8_t *decompress(uint8_t *cdata, uint32_t csize, uint32_t size) {
  uint8_t *data = malloc(size+1);
  ulz_decode(cdata, csize, data, size);
  data[size] = 0; //makes text files easier to process
  return data;
}

#else

static uint8_t *compress(uint32_t *osize, uint8_t *data, uint32_t size) {
  uint8_t *cdata = malloc(size);
  memcpy(cdata, data, size);
  if (osize) *osize = size;
  return cdata;
}

static uint8_t *decompress(uint8_t *cdata, uint32_t csize, uint32_t size) {
  uint8_t *data = malloc(size);
  memcpy(data, cdata, size);
  return data;
}

#endif


typedef struct {
  char *name;
  uint32_t size;
  uint32_t csize;
  uint32_t ofs;
  uint32_t flags;
} ft_entry_t;

typedef struct {
  char *path;
  FILE *file; //closed if 0
  uint64_t atime; //last access time
  struct { char *key; ft_entry_t *value; } *ft;
} saf_t;

typedef struct {
  uint32_t pos; //position inside file
  ft_entry_t *fte;
  saf_t *saf;
} saf_file_t;


static saf_t **safs;

#define SAF_FOLDER      0x01
#define SAF_COMPRESSED  0x02

int saf_create(char *saf_name, char *path) {
  int pl = strlen(path);
  if (path[pl-1] != '/' && path[pl-1] != '\\') path = cat(path,"/");
  else path = strdup(path);
  pl = strlen(path);

  if (!path_is_folder(path)) {
    fprintf(stderr, "Couldn't read %s\n", path);
    return -1;
  }


  char **tlist = list_tree(path);
  arrput(tlist, strdup(path));

  ft_entry_t *es = 0; //file table entries
  uint32_t ofs = 0;
  
  FILE *saf = fopen(saf_name, "wb");

  for (int i = 0; i < arrlen(tlist); i++) {
    char *n = tlist[i];
    int nl = strlen(n);
    int is_folder = n[nl-1]=='/' || n[nl-1]=='\\';
    int64_t s;
    int data_isarr = 0;
    uint8_t *data = 0;
    if (is_folder) {
      char **fs = list_folder(n,0);
      data_isarr = 1;
      for (int j = 0; j < arrlen(fs); j++) {
        for (char *p = fs[j]; *p; p++) arrput(data, (uint8_t)*p);
        arrput(data,'\n');
        free(fs[j]);
      }
      arrfree(fs);
      s = arrlen(data);
    } else {
      data = read_whole_file(n, &s);
    }
    ft_entry_t e;
    e.ofs = ofs;
    e.size = (uint32_t)s;
    e.name = strdup(n+pl);
    e.flags = 0;
    e.flags |= SAF_COMPRESSED;
    if (is_folder) e.flags |= SAF_FOLDER;
    uint8_t *cdata = compress(&e.csize, data, e.size);
    fwrite(cdata, 1, e.csize, saf);
    if (data_isarr) arrfree(data);
    else free(data);
    free(cdata);
    free(n);
    ofs += e.csize;
    arrpush(es, e);
    //int c = e.size ? e.csize*100/e.size : 100;
    //fprintf(stderr, "%s:\n %d (%d/%d)\n", e.name, c, e.csize, e.size);
  }

  arrfree(tlist);

  uint32_t fto = ofs;

  uint8_t *wb = 0;

  int nentries = arrlen(es);
  //fprintf(stderr, "nentries=%d\n", nentries);
  for (int i = 0; i < nentries; i++) {
    ft_entry_t e = es[i];
    EMIT32(e.size);
    EMIT32(e.ofs);
    EMIT32(e.flags);
    for (char *p = e.name; *p; p++) EMIT8(*p);
    EMIT8(0);
    //fprintf(stderr, "%s\n", e.name);
  }

  uint32_t ftsz = arrlen(wb);
  uint32_t cftsz;
  uint8_t *cft = compress(&cftsz, wb, arrlen(wb));
  arrfree(wb);
  fwrite(cft, 1, cftsz, saf);

#define HDRSZ 20
  uint8_t *key = 0;
  if (cftsz<HDRSZ) {
    for (int i = 0; i < cftsz; i++) arrput(key, cft[i]);
    for (int i = 0; i < HDRSZ-cftsz; i++) arrput(key, 0xFF);
    fwrite(key, 1, arrlen(key), saf);
  } else {
    for (int i = cftsz-HDRSZ; i < cftsz; i++) arrput(key, cft[i]);
  }


  wb = 0;

  EMIT8(1);
  EMIT8('S');
  EMIT8('A');
  EMIT8('F');
  EMIT32(fto);
  EMIT32(ftsz);  
  EMIT32(nentries);
  EMIT32(0); //flags

  for (int i = 0; i < HDRSZ; i++) wb[i] ^= key[i];

  fwrite(wb, 1, arrlen(wb), saf);
  arrfree(wb);

  free(cft);
  free(path);
  arrfree(key);
  fclose(saf);
  
  return 0;
}

saf_t *saf_open(char *saf_path);

int saf_extract(char *saf_name, char *path) {
  int pl = strlen(path);
  if (path[pl-1] != '/' && path[pl-1] != '\\') path = cat(path,"/");
  else path = strdup(path);
  pl = strlen(path);

  saf_t *s = saf_open(saf_name);
  if (!s) {
    fprintf(stderr, "Could not open `%s`\n", saf_name);
    return -1;
  }

  for (int i = 0; i < shlen(s->ft); i++) {
    ft_entry_t e = *s->ft[i].value;
    if (e.flags&SAF_FOLDER) {
      //fprintf(stderr, "%s\n  %d,%d,%d\n", e.name, e.size, e.ofs, e.flags);
      continue;
    }
    //sfprintf(stderr, "%s\n", e.name);
    char *t = cat(path,e.name);
    mkpath(t, NG_SKIPFN);

    uint8_t *buf = saf_get(0, saf_name, e.name);
    if (buf) {
      FILE *o = fopen(t, "wb");
      if (o) {
        fwrite(buf, 1, e.size, o);
        fclose(o);
      } else {
        fprintf(stderr, "Could not write `%s`\n", t);
      }
      free(buf);
    } else {
      fprintf(stderr, "Could not extract `%s`\n", e.name);
    }
    free(t);
  }
  return 0;
}


void *saf_new(char *saf_path) {
  FILE *f = fopen(saf_path, "rb");
  if (!f) return 0;

  fseek(f, -HDRSZ*2, SEEK_END);
  int32_t safsz = ftell(f)+HDRSZ;
  uint8_t hdr[HDRSZ*2];
  fread(hdr, 1, HDRSZ*2, f);

  uint8_t *pin = hdr+HDRSZ;
  for (int i = 0; i < HDRSZ; i++) pin[i] ^= hdr[i];

  if (pin[0] != 1 || pin[1] != 'S' || pin[2] != 'A' || pin[3] != 'F') {
    fclose(f);
    return 0;
  }

  saf_t *s = malloc(sizeof(saf_t));
  s->file = f;
  s->path = strdup(saf_path);
  s->atime = clock();
  s->ft = 0;

  pin += 4;
  int32_t fto = RD32;
  int32_t ftsz = RD32;
  int32_t nentries = RD32;
  int32_t flags = RD32;
  int32_t cftsz = safsz-fto;

  uint8_t *cft = malloc(cftsz);
  fseek(f, fto, SEEK_SET);
  fread(cft, 1, cftsz, f);


  uint8_t *ft = decompress(cft, cftsz, ftsz);
  free(cft);

  pin = ft;
  ft_entry_t *pe = 0;
  //fprintf(stderr, "nentries=%d\n", nentries);
  for (int i = 0; i < nentries; i++) {
    ft_entry_t *e = malloc(sizeof(ft_entry_t));
    e->size = RD32;
    e->ofs = RD32;
    e->flags = RD32;
    e->name = strdup((char*)pin);
    //fprintf(stderr, "%s\n", e->name);
    pin += strlen((char*)pin)+1;
    shput(s->ft,e->name,e);
    if (pe) pe->csize = e->ofs - pe->ofs;
    pe = e;
  }

  if (pe) pe->csize = fto - pe->ofs;

  free(ft);
  
  return s;
}

saf_t *saf_open(char *saf_path) {
  saf_t *s = 0;
  for (int i = 0; i < arrlen(safs); i++) {
    if (!strcmp(safs[i]->path, saf_path)) {
      s = safs[i];
      break;
    }
  }

  if (!s) {
    s = saf_new(saf_path);
    if (!s) {
      fprintf(stderr, "Coldn't open `%s`\n", saf_path);
      return 0;
    }
    arrput(safs,s);
  }
  
  if (!s->file) {
    fprintf(stderr, "implement reopening closed archives\n");
    return 0;
  }
  return s;
}

uint8_t *saf_get(int64_t *fsize, char *saf_path, char *path) {
  saf_t *s = saf_open(saf_path);

  ft_entry_t *e = shget(s->ft,path);
  if (!e) return 0;

  uint8_t *cdata = malloc(e->csize);
  fseek(s->file, e->ofs, SEEK_SET);
  fread(cdata, 1, e->csize, s->file);
  uint8_t *data = decompress(cdata, e->csize, e->size);
  free(cdata);
  if (fsize) *fsize = e->size;

  return data;
}

int saf_is_folder(char *saf_path, char *path) {
  saf_t *s = saf_open(saf_path);
  
  int pl = strlen(path);
  int has_slash = !pl || (path[pl-1] == '/');
  if (!has_slash) path = cat(path,"/");

  ft_entry_t *e = shget(s->ft, path);
  if (!has_slash) free(path);
  if (!e) return 0;
  return e->flags & SAF_FOLDER;
}


int saf_is_file(char *saf_path, char *path) {
  saf_t *s = saf_open(saf_path);

  int pl = strlen(path);
  int has_slash = !pl || (path[pl-1] == '/');
  if (has_slash) return 0;
  ft_entry_t *e = shget(s->ft, path);
  if (!e) return 0;
  return 1;
}

#undef SAF_IMPLEMENTATION
#endif
