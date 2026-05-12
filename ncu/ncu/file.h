#ifndef NCU_FILE_H
#define NCU_FILE_H

#include "base.h"

#define NCU_FILE_SIZE_ERROR 0xFFFFFFFF

U32 fileSize(char *filename);
U8 *fileGet(U32 *rsize, char *filename);
int fileSet(char *filename, U8 *data, U32 size);

#endif