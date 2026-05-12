#ifndef FS_H
#define FS_H

#include <stdint.h>

#define MNT_SAF 0x01

void fs_mount(char *src, char *dst, uint32_t flags);
char *fs_resolve(char **saf_name, char *path);
int fs_is_file(char *path);
int fs_is_folder(char *path);
uint64_t fs_time(char *path);
int fs_exists(char *path);
uint8_t *fs_get(int64_t *size, char *path);
int fs_copy(char *src, char *dst);

/*

  - To access an item inside SAF archive,
    it has to be mounted first as a <mount_prefix>
    then usual functions will access the file through:
      `<mount_prefix>/path/to/file`

*/



#endif
