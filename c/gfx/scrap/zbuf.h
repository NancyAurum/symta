/*

typedef struct {
  U32 w;
  U32 h;
  float data[];
} zbuf_t;


ffi new_zbuf.ptr W.u4 H.u4
ffi free_zbuf.void ZBuf.ptr
ffi zbuf_w.u4 ZBuf.ptr
ffi zbuf_h.u4 ZBuf.ptr
ffi zbuf_get.float ZBuf.ptr X.int Y.int
ffi zbuf_set.void ZBuf.ptr X.int Y.int Value.float
ffi zbuf_clear.void ZBuf.ptr Value.float
ffi zbuf_init.void ZBuf.ptr SrcZBuf.ptr

ffi gfx_zcompose.void Dst.ptr DstZBuf.ptr Src.ptr SrcZBuf.ptr


type zbuf{w h} handle
| $handle <= new_zbuf 1 1

zbuf.clear Value = zbuf_clear $handle  Value

zbuf.resize W H =
| when $handle: free_zbuf $handle 
| $handle <= new_zbuf W H
| $w <= W
| $h <= H

zbuf.init SrcZBuf = zbuf_init $handle SrcZBuf.handle


gfx.zcompose ZBuf Src SrcZBuf =
| gfx_zcompose $handle ZBuf.handle Src.handle SrcZBuf.handle
*/



//FIXME: this stuff should be moved into gfx

zbuf_t *new_zbuf(U32 w, U32 h) {
  zbuf_t *zbuf = (zbuf_t*)malloc(sizeof(zbuf_t)+w*h*sizeof(float));
  if (!zbuf) return 0;
  zbuf->w = w;
  zbuf->h = h;
  return zbuf;
}

void free_zbuf(zbuf_t *zbuf) {
  free(zbuf);
}

U32 zbuf_w(zbuf_t *zbuf) {
  return zbuf->w;
}

U32 zbuf_h(zbuf_t *zbuf) {
  return zbuf->h;
}

float zbuf_get(zbuf_t *zbuf, U32 x, U32 y) {
  return zbuf->data[y*zbuf->w + x];
}

void zbuf_set(zbuf_t *zbuf, U32 x, U32 y, float value) {
  zbuf->data[y*zbuf->w + x] = value;
}


void zbuf_clear(zbuf_t *zbuf, float value) {
  U32 v = *(U32*)&value;
  memset4((U32*)zbuf->data, v, zbuf->w*zbuf->h);
}

void zbuf_init(zbuf_t *zbuf, zbuf_t *src) {
  memcpy4((U32*)zbuf->data, (U32*)src->data, zbuf->w*zbuf->h);
}

void gfx_zcompose(gfx_t *dst, zbuf_t *dstzb
                 ,gfx_t *src, zbuf_t *srczb) {
  U32 w = dst->w;
  U32 h = dst->h;
  U32 i;
  U32 iend = w*h;
  U32 *sp = src->data;
  U32 *dp = dst->data;
  float *szp = srczb->data;
  float *dzp = dstzb->data;
  for (i = 0; i < iend; i++) {
    if (!(sp[i]&0xFF000000) && dzp[i] > szp[i]) {
      dzp[i] = szp[i];
      dp[i] = sp[i];
    }
  }
}
