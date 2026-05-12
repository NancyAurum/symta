#include <stdio.h>
#include <stdarg.h>
#include <float.h>
#include <math.h>
#include <dirent.h>

#define SAF_IMPLEMENTATION
#include "saf.h"

#define NG_IMPLEMENTATION
#include "ng.h"

static int print_usage() {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  saf c <dst.saf> <src_folder>\n");
  fprintf(stderr, "  saf x <src.saf> <dst_folder>\n");
  return -1;
}

void test_saf() {
  int64_t size;
  uint8_t *data = saf_get(&size, "dst.saf", "main/site.txt");
  if (!data) {
    fprintf(stderr, "Couldn't read the file!!\n");
    return;
  }
  fprintf(stderr, "size=%d\n", (int)size);
  fprintf(stderr, "Data\n%s\n", data);
}

int main(int argc, char **argv) {
#if 0
  test_saf();
  return 0;
#endif

  if (argc != 4) return print_usage();

  char *saf_name = argv[2];
  char *path = argv[3];

  if (!strcmp(argv[1], "c")) return saf_create(saf_name, path);
  else if (!strcmp(argv[1], "x")) return saf_extract(saf_name, path);
  else return print_usage();

  return -1;
}

