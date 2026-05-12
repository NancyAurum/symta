//Macros for create growing list structure holding elements of any type
//Copyright (c) 2020 Nash Gold
//All rights reserved.

#ifndef NCU_LST_H
#define NCU_LST_H

#include "base.h"

#define LST_INIT_SZ 4

#define LST_INIT {0,0,0}

#define lst_dcl(type,elt_type)              \
typedef struct {                            \
  elt_type *elts;                           \
  uint32_t size;                            \
  uint32_t used;                            \
} type;                                     \
NCU_INLINE void type##Init(type *lst) {                               \
  lst->used = 0;                                                      \
  lst->size = LST_INIT_SZ;                                            \
  lst->elts = (elt_type *)malloc(sizeof(lst->elts[0])*LST_INIT_SZ);   \
}                                                                     \
NCU_INLINE void type##InitN(type *lst, int sz) {                      \
  lst->used = 0;                                                      \
  lst->size = sz;                                                     \
  lst->elts = (elt_type *)malloc(sizeof(lst->elts[0])*sz);            \
}                                                                     \
NCU_INLINE void type##Free(type *lst) { \
  if (!lst->size) return;               \
  free(lst->elts);                      \
  lst->used = 0;                        \
  lst->size = 0;                        \
  lst->elts = 0;                        \
}                                       \
static void type##Grow(type *lst) {                          \
  if (!lst->size) {                                          \
    type##Init(lst);                                         \
    return;                                                  \
  }                                                          \
  uint32_t new_size = lst->size*2;                           \
  uint32_t alcsz = sizeof(lst->elts[0])*new_size;            \
  elt_type *new_elts = (elt_type *)malloc(alcsz);            \
  memcpy(new_elts,lst->elts,sizeof(lst->elts[0])*lst->size); \
  free(lst->elts);                                           \
  lst->elts = new_elts;                                      \
  lst->size = new_size;                                      \
}                                                            \
NCU_INLINE uint32_t type##Reserve(type *lst) { \
  if (lst->used==lst->size) type##Grow(lst);   \
  return lst->used++;                          \
}                                              \
NCU_INLINE elt_type *type##Add(type *lst) { \
  uint32_t i = type##Reserve(lst); \
  return &lst->elts[i];    \
}                                           \
NCU_INLINE elt_type type##Get(type *lst, uint32_t i) { \
  return lst->elts[i]; \
} \
NCU_INLINE elt_type *type##Ref(type *lst, uint32_t i) { \
  return &lst->elts[i]; \
} \
NCU_INLINE elt_type *type##ERef(type *lst, uint32_t i) { \
  while (lst->size <= i) type##Grow(lst);   \
  return &lst->elts[i]; \
} \
NCU_INLINE void type##Push(type *lst, elt_type value) { \
  *type##Add(lst) = value; \
} \
NCU_INLINE elt_type type##Pop(type *lst) { \
  return lst->elts[--lst->used]; \
} \
NCU_INLINE elt_type *type##Top(type *lst) { \
  return &lst->elts[lst->used-1]; \
} \

#define lst_for(x,xs)                              \
  for (int i_ = 0, sz_=(xs)->used, j_; i_ < sz_; i_++, j_=1) \
    for (x=(xs)->elts[i_]; j_; j_=0)


#if 0

#include "../include/lst.h"

  
static char *months[] = {
   "jan","feb","mar","apr"
  ,"may","jun","jul","aug"
  ,"sep","oct","nov","dec"
  ,0
};

lst_dcl(slst,char *) 

void lst_test() {
  slst xs = LST_INIT;

  printf("______POPULATING_____\n");
  for (int i = 0; months[i]; i++) {
    printf("adding %s (%d)\n", months[i], i);
    *slstAdd(&xs) = months[i];
  }

  printf("______RETRIEVED______\n");
  lst_for(char *x, &xs) {
    printf("%d: %s\n", i_, x);
  }
  printf("________TABLE________\n");
  printf("size=%d/%d\n", xs.used, xs.size);

  lst_for(char *x, &xs) {
    printf("%s = %d\n", x, i_);
  }
}

#endif


#if 0
//define the list structure with the element type el_type
#define lst_def(type_name,el_type) \
  typedef struct {   \
    el_type *elts;   \
    uint32_t used;   \
    uint32_t size;   \
  } type_name

#define lst_init(lst,init_size) do { \
    (lst)->size = init_size; \
    *(void**)&((lst)->elts) = malloc(sizeof((lst)->elts[0])*(lst)->size); \
    (lst)->used = 0; \
  } while (0)


#define lst_free(lst) do { \
      if ((lst)->elts) {    \
        free((lst)->elts);  \
        (lst)->elts = 0;    \
      }                    \
  } while (0)

// adds element to the list, returns its index as `result`
#define lst_add(result,lst) do { \
    if ((lst)->used==(lst)->size) { \
      uint32_t new_size_ = (lst)->size*2; \
      uint32_t alcsz_ = sizeof((lst)->elts[0])*new_size_; \
      void *new_elts_ = (void*)malloc(alcsz_); \
      memcpy(new_elts_,(lst)->elts,sizeof((lst)->elts[0])*(lst)->size); \
      free((lst)->elts); \
      *(void**)&((lst)->elts) = new_elts_; \
      (lst)->size = new_size_; \
    } \
  } while (0); \
  uint32_t result = (lst)->used++;

//expanding set
#define lst_eset(lst,index,value) do { \
    uint32_t index_ = (index); \
    while ((lst)->size <= index_) { \
      uint32_t new_size_ = (lst)->size*2; \
      uint32_t alcsz_ = sizeof((lst)->elts[0])*new_size_; \
      void *new_elts_ = (void*)malloc(alcsz_); \
      memcpy(new_elts_,(lst)->elts,sizeof((lst)->elts[0])*(lst)->size); \
      free((lst)->elts); \
      *(void**)&((lst)->elts) = new_elts_; \
      (lst)->size = new_size_; \
    } \
    (lst)->elts[index_] = (value); \
    if ((lst)->used <= index_) (lst)->used = index_+1; \
  } while (0);

//get list item
#define lst_get(lst,index) ((lst)->elts[index])



#define lst_push(lst,item) do {     \
    lst_add(index_,(lst));          \
    lst_get((lst),index_) = (item); \
  } while (0)

#define lst_top(lst) ((lst)->elts[(lst)->used-1])


//a few default lists
lst_def(uint8_t_lst_t,uint8_t);
lst_def(uint16_t_lst_t,uint16_t);
lst_def(uint32_t_lst_t,uint32_t);
lst_def(uint64_t_lst_t,uint64_t);
lst_def(int8_t_lst_t,int8_t);
lst_def(int16_t_lst_t,int16_t);
lst_def(int32_t_lst_t,int32_t);
lst_def(int64_t_lst_t,int64_t);
lst_def(cstr_t_lst_t,char*);
#endif

#endif
