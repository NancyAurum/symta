#include <stdio.h>
#include "file.h"



U32 fileSize(char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) return NCU_FILE_SIZE_ERROR;
  fseek(fp, 0L, SEEK_END);
  U32 sz = ftell(fp);
  fclose(fp);
  return sz;
}

U8 *fileGet(U32 *rsize, char *filename) {
  U32 sz = fileSize(filename);
  if (sz == NCU_FILE_SIZE_ERROR) return 0;
  FILE *fp = fopen(filename, "rb");
  if (!fp) return 0;
  *rsize = sz;
  uint8_t *p = malloc(sz + 1);
  p[sz] = 0;
  fread(p, 1, sz, fp);
  fclose(fp);
  return p;
}

int fileSet(char *filename, U8 *data, U32 size) {
  FILE *fp = fopen(filename, "wb");
  if (!fp) return 0;
  fwrite(data, 1, size, fp);
  fclose(fp);
  return 1;
}
