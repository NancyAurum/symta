#ifndef SRFFI_H
#define SRFFI_H


#include <stdint.h>


//exports parts of symta's runtime to foreign code
typedef struct {
  void (*fatal)(char *fmt, ...);

  void *(*ffi_load)(char *lib_name, char *sym_name);

  char *(*fs_resolve)(char **saf_name, char *path);
  int (*fs_is_file)(char *path);
  int (*fs_is_folder)(char *path);
  uint64_t (*fs_time)(char *path);
  int (*fs_exists)(char *path);
  uint8_t *(*fs_get)(int64_t *size, char *path);
  int (*fs_copy)(char *src, char *dst);
} effi_t;



#endif //SRFFI_H
