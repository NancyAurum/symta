#include <ctype.h>
#include <float.h>

#include "sif.h"
#include "ng.h"
#include "fs.h"
#include "sffi/sffi.h"

#define MCACHE_DIV 4

sbc_t *sbc_new(uint8_t *pin, int64_t size, char *path) {
  sbc_t *sbc = malloc(sizeof(sbc_t));
  sbc->file = pin;

  //FIXME: better idea would be storing these as offsets
  uint32_t descriptor = RD16;
  sbc->src_file_string = RD24;
  sbc->deps_string = RD24;
  sbc->export_string = RD24;
  uint32_t tot_sz = RD16;
  sbc->code_sz = RD24;
  sbc->data_sz = RD24;
  sbc->tbls_sz = RD24;
  sbc->fntbl_sz = RD24;
  sbc->tot = pin;
  pin += tot_sz*3*2;

  sbc->code = pin;
  pin += sbc->code_sz;
  sbc->data = pin;
  pin += sbc->data_sz;
  sbc->tbls = pin;
  pin += sbc->tbls_sz;
  sbc->fntbl = pin;
  pin += sbc->fntbl_sz;
  
  pin = sbc->tot + 3*2*7;
  sbc->nrs_sz = RD24;
  sbc->nrs = sbc->tbls + RD24;

  sbc->filename = strdup(path);
  sbc->mt = 0;
  sbc->hooks = 0;
  sbc->ready = 0;
  sbc->nfi_ctx = 0;
  sbc->nfi_trmps = 0;
  //fprintf(stderr, "%s:\n  %s\n", path,sbc->code+sbc->export_string);
  return sbc;
}

sbc_t *sbc_load(char *path) {
  int64_t size;
  uint8_t *pin = fs_get(&size, path);
  if (!pin) return 0;
  return sbc_new(pin, size, path);
}

void sbc_free(sbc_t *sbc) {
  if (sbc->ready) {
    free(sbc->mt);
    if (sbc->hooks) free(sbc->hooks);
    for (int i = 0; i < 7; i++) {
      //FIXME: ensure it gets unloaded from the runtime
      if (sbc->rtot[i].table) free(sbc->rtot[i].table);
    }
    /* sffi sigs were allocated one-per-FFI at sbc_prepare. Release
     * them here. nfi_ctx isn't used by sffi (it was cinvoke's
     * context handle); we keep the field in sbc_t for ABI
     * compatibility with code that still reads it, but it's now
     * always NULL. */
    if (sbc->nfi_trmps) {
      for (int i = 0; i < arrlen(sbc->nfi_trmps); i++) {
        sffi_free((sffi_sig_t *)sbc->nfi_trmps[i]);
      }
      arrfree(sbc->nfi_trmps);
    }
  }
  if (sbc->filename) free(sbc->filename);
  free(sbc->file);
  free(sbc);
}

int sbc_load_metadata(char *filename, char **src, char **deps, char **export) {
  sbc_t *sbc = sbc_load(filename);
  if (!sbc) return 0;
  *src = strdup(sbc->code+sbc->src_file_string);
  *deps = strdup(sbc->code+sbc->deps_string);
  *export = strdup(sbc->code+sbc->export_string);
  sbc_free(sbc);
  return 1;
}

#ifdef NATIVE_HOOKS
//basically curries the 2nd argument of a 2-argument function
uint8_t *emit_hook(uint8_t *dst, void *target, void *payload) {
  //OSX functions use RDI to pass 1st argument, and RSI to pass 2nd
  uint8_t *wb = dst;
  EMIT8(0x48); //mov rax, fn
  EMIT8(0xB8);
  EMIT64((uint64_t*)target);

  EMIT8(0x48); //mov rsi, payload
  EMIT8(0xBE);
  EMIT64((uint64_t*)payload);

#if 0
  EMIT8(0x48); //mov rdi, payload
  EMIT8(0xBF);
  EMIT64((uint64_t*)payload);
#endif

#if 0
  //following can be useful, if we instead prefix the payload to the buf
  //or put the curry code before the interpreted function
  EMIT8(0xE8); //push ip
  EMIT32(0);
  EMIT8(0x5F);  //pop rdi
#endif

  EMIT8(0xFF);  //jmp rax
  EMIT8(0xE0);

  return wb;
}
#endif

#define MAX_HOOKS (1024*64)

hook_t hooks_heap[MAX_HOOKS];
hook_t *hooks_ptr = hooks_heap+2; //hook 0 is local scope closures

uint32_t sbc_hook(psf_t fn, uint8_t *payload) {
  uint32_t r = hooks_ptr-hooks_heap;

  FAIL(hooks_ptr+1 >= hooks_heap+MAX_HOOKS , "Hooks heap overflow\n");

  hooks_ptr->handler = fn;
  hooks_ptr->payload = payload;
  hooks_ptr->meta = 0;
  hooks_ptr++;
  return r;
}

void sbc_emit_hooks(sbc_t *sbc, int sbc_id) {
  uint8_t *pin = sbc->fntbl;
  int nfns = sbc->fntbl_sz/3;
  sbc->hooks = malloc(sizeof(uint64_t)*nfns);
  for (int i = 0; i < nfns; i++) {
    uint32_t ofs = RD24;
    uint8_t *fptr = sbc->code + ofs;
    fptr[1] = sbc_id&0xFF;      //patch subr
    fptr[2] = (sbc_id>>8)&0xFF;
    sbc->hooks[i] = sbc_hook(&sbc_exec_fn, fptr+1);
  }
  
  //make_executable(sbc->hooks_base,nfns*sizeof(hook_t));
}


void sbc_init_tables(sbc_t *sbc) {
  int i,j,k;
  tot_entry_t *rtot = sbc->rtot;

  uint8_t *totpin = sbc->tot;
  uint8_t *pin;
  char *tns[7] = {"fm","tx","ty","mt","libs","imlib","im"};
  int code_sz = sbc->code_sz;
  for (i = 0; i < 7; i++) {
    pin = totpin;
    int nentries = RD24;
    int tofs = RD24;
    totpin = pin;
    pin = sbc->tbls+tofs;
    rtot[i].size = nentries;
    if (i == 0) { //fnmeta?
      fn_meta_t *t = malloc(sizeof(fn_meta_t)*nentries);
      rtot[i].table = t;
      for (j = 0; j < nentries; j++) {
        uint32_t ofs;
        int64_t nargs = RD16;
        if (nargs == 0xFFFF) nargs = -1;
        t[j].nargs = FXN(nargs);
  
        ofs = RD24;
        if (ofs) t[j].name = sbc->data + ofs-code_sz;
        else t[j].name = 0;

        int idx = RD16;
        t[j].hook = (uint32_t)sbc->hooks[idx];


        //FIXME: upper col bit could specify if 
        //       src differs from the sbc->src_file_string
        t[j].row = RD16;
        t[j].col = RD16;

        if (t[j].col&SBC_DEFAULT_SRC) {
          ofs = sbc->src_file_string;
          t[j].col &= ~SBC_DEFAULT_SRC;
        } else {
          ofs = RD24;
        }
        if (ofs) t[j].origin = sbc->data + ofs-code_sz;
        else t[j].origin = 0;
      }
    } else {
      dyn *t = malloc(sizeof(dyn)*nentries);
      rtot[i].table = t;
      for (j = 0; j < nentries; j++) {
        if (i != 5) {
          uint32_t ofs = RD24;
          if (ofs) t[j] = sbc->data + ofs-code_sz;
          else t[j] = 0;
        } else {
          t[j] = (dyn)(uint64_t)RD16; //library index
        }
      }
    }
  }

  sbc_init_types(sbc, rtot);
}

#define SBCS_MAX 1024
static int sbcs_loaded = 0;
static sbc_t *sbcs[SBCS_MAX];

/* SBC_NFI_* opcode → sffi_type. Returns -1 on unrecognised. */
static int sbc2sffi(int id) {
  switch (id) {
  case SBC_NFI_VOID: return SFFI_VOID;
  case SBC_NFI_INT:  return SFFI_I32;
  case SBC_NFI_PTR:  return SFFI_PTR;
  case SBC_NFI_TXT:  return SFFI_PTR;  /* char* — same ABI as void* */
  case SBC_NFI_U4:   return SFFI_U32;
  case SBC_NFI_FLT:  return SFFI_F32;
  case SBC_NFI_DBL:  return SFFI_F64;
  }
  return -1;
}

void sbc_prepare(sbc_t *sbc) {
  int sbc_id = sbcs_loaded;
  if (sbc_id == SBCS_MAX) {
    fprintf(stderr, "sbc_prepare: sbcs[] overflow\n");
    exit(-1);
  }
  sbcs[sbcs_loaded++] = sbc;
  sbc_emit_hooks(sbc, sbc_id);

  /* sffi has no per-SBC context (was a cinvoke thing). Keep the
   * field NULL; nothing else reads it. */
  sbc->nfi_ctx = 0;

  /* Walk the per-FFI argument-type stream and bind a sffi sig for
   * each. Stored in sbc->nfi_trmps[] so SBC_NFI invocations can
   * grab the sig by index. */
  uint8_t *pin = sbc->nrs;
  for (int i = 0; i < sbc->nrs_sz; i++) {
    sffi_type arg_types[SFFI_MAX_ARGS];
    int n_args = 0;
    int ofs = RD24;
    uint8_t *p = sbc->code + ofs;
    uint8_t *e = sbc->code + sbc->code_sz;
    while (*p != SBC_NFI && p < e) {
      int t = sbc2sffi(*p);
      if (t < 0 || n_args >= SFFI_MAX_ARGS) {
        fprintf(stderr, "sbc_prepare: bad SBC_NFI arg type 0x%02x "
                        "in %s\n", *p, sbc->filename);
        exit(-1);
      }
      arg_types[n_args++] = (sffi_type)t;
      p += 3;
    }
    p += 5;
    int rt = sbc2sffi(*p);
    if (rt < 0) {
      fprintf(stderr, "sbc_prepare: bad SBC_NFI return type 0x%02x "
                      "in %s\n", *p, sbc->filename);
      exit(-1);
    }
    sffi_sig_t *sig = sffi_bind((sffi_type)rt, n_args, arg_types);
    if (!sig) {
      fprintf(stderr, "sbc_prepare: sffi_bind failed for FFI %d "
                      "in %s\n", i, sbc->filename);
      exit(-1);
    }
    arrput(sbc->nfi_trmps, sig);
  }

  sbc_init_tables(sbc);

  sbc->ready = 1;
}

#if 1
#define CHKREG(x) 
#else
#define CHKREG(x) \
  if ((x)>=nvars) do { \
    fprintf(stderr,"%s:%d: regs overflow, %d >= %d\n"\
      ,__FILE__,__LINE__,x,nvars); \
    fprintf(stderr,"sbc file:%06x: %s\n", (int)(pin-sbc->code), sbc->filename); \
    volatile int x = 0; \
    volatile int y = 1; \
    y /= x; \
    exit(-1); \
  } while(0)
#endif


//#define MCACHE_PROFILE

#ifdef MCACHE_PROFILE
int64_t mcache_misses = 0;
int64_t mcache_hits = 0;
#define MCACHE_HIT do { \
    if ((++mcache_hits % 100000) == 0) \
      printf("hits %lld: %06x\n", mcache_hits, ip); \
 } while (0);
#define MCACHE_MISS do { \
    if ((++mcache_misses % 100000) == 0) \
      printf("misses %lld: %06x\n", mcache_misses, ip); \
 } while (0);
#else
#define MCACHE_HIT
#define MCACHE_MISS
#endif

/*
  FIXME: implement a probation scheme, where mcalls whose object type changes
         frequently get patched with normal MCALLS.
         i.e. maintain ration of hits to misses.
*/

#define MCACHE_SKIP pin += sizeof(mcache_t)

#ifdef SBC_MCACHE

//#define MCACHE_CALL(k,o,mid) MCALL(k,o,mid) 
#define MCACHE_CALL(k,o,mid) do {            \
    int m = mid;                            \
    api.method = m;                         \
    dyn oo = o;                             \
    mcache_t *mce = (mcache_t*)pin;         \
    MCACHE_SKIP;                            \
    uint32_t tid = O_TAG(o);                \
    method_node_t *node = mce->node;        \
    if (!node || node->tid != tid) {        \
      node = get_method_node(m,tid);        \
      mce->node = node;                     \
      MCACHE_MISS                           \
    } else {MCACHE_HIT}                     \
    dyn mfn = node->fn;                     \
    CALL(k,mfn);                            \
  } while (0)
#else
#define MCACHE_CALL(k,o,mid) MCALL(k,o,mid) 
#endif



int sbcd = 0; //SBC debug

int sbc_opc;
int64_t sbc_icount;

dyn sbc_exec_fn(uint8_t *pin) {
#if 0
  if (pin[-1] != SBC_SUBR) {
    fatal("sbc_exec_fn: first opcode is not SUBR");
  }
#endif

  //FIXME: it would be more efficient to store the sbc_t * pointer
  sbc_t *sbc = sbcs[RD16];

  int nvars = RD16;
  SUBR(nvars);
  
  for (;;) {
  switch(RD8) {
  case SBC_NOP: {break;}
  case SBC_LEAVE0: return (dyn)0;
  case SBC_LEAVEN: {return FXN((int64_t)RD16);}
  case SBC_LEAVE: {
    int src = RD16;
    //fprintf(stderr, "leave: %d\n", src);
    CHKREG(src);
    return L[src];}
  case SBC_JMP: {pin = sbc->code+RD24; break;}
  case SBC_JMP16: {
    int16_t diff = (int16_t)RD16;
    pin += diff;
    break;}
  case SBC_B: {
    uint32_t cnd = RD16;
    uint32_t ofs = RD24;
    CHKREG(cnd);
    if (L[cnd]) pin = sbc->code+ofs;
    break;}
  case SBC_B8: {
    uint32_t cnd = RD8;
    int16_t diff = (int16_t)RD16;
    CHKREG(cnd);
    if (L[cnd]) pin += diff;
    break;}
  case SBC_IFFXN: {
    uint32_t cnd = RD8;
    int16_t diff = (int16_t)RD16;
    CHKREG(cnd);
    if (!O_TAG(L[cnd])) pin += diff;
    break;}
  case SBC_CALL: {
    int dst = RD16;
    int fn = RD16;
    CHKREG(dst);
    CHKREG(fn);
    CALL(L[dst],L[fn]);
    break;}
  case SBC_CALLIR: {
    int fn = RD16;
    dyn dummy;
    CHKREG(fn);
    CALL(dummy,L[fn]);
    break;}
  case SBC_CALLT: {
    int dst = RD16;
    int fn = RD16;
    CHKREG(dst);
    CHKREG(fn);
    CALL_TAGGED(L[dst],L[fn]);
    break;}
  case SBC_CALLTIR: {
    int fn = RD16;
    dyn dummy;
    CHKREG(fn);
    CALL_TAGGED(dummy,L[fn]);
    break;}
  case SBC_TCALL: {
    int fn = RD16;
    dyn *r;
    CHKREG(fn);
    CALL(r,L[fn]);
    //FIXME: instead check that thunk handler is sbc_exec_fn and jump to the
    //       beginning
    return r;
    break;}
  case SBC_MCALL: {
    int dst = RD16;
    int obj = RD16;
    int met = RD16;
    CHKREG(dst);
    CHKREG(obj);
    MCACHE_CALL(L[dst],L[obj],sbc->mt[met]);
    break;}
  case SBC_MCALLIR: {
    dyn dummy;
    int obj = RD16;
    int met = RD16;
    CHKREG(obj);
    MCACHE_CALL(dummy,L[obj],sbc->mt[met]);
    break;}
  case SBC_MCALL8: {
    int dst = RD8;
    int obj = RD8;
    int met = RD8;
    CHKREG(dst);
    CHKREG(obj);
    MCACHE_CALL(L[dst],L[obj],sbc->mt[met]);
    break;}
  case SBC_TMCALL: {
    int obj = RD16;
    int met = RD16;
    dyn *r;
    CHKREG(obj);
    MCACHE_CALL(r,L[obj],sbc->mt[met]);
    //FIXME: instead check that thunk handler is sbc_exec_fn and jump to the
    //       beginning
    return r;
    break;}
  case SBC_CLOSURE: {
    uint32_t dst = RD16;
    uint32_t idx = RD16;
    uint32_t size = RD8;
    CHKREG(dst);
    void *fn = (void*)sbc->hooks[idx];
    CLOSURE(L[dst],fn,size)
    break;}
  case SBC_OBJECT: {
    int dst = RD16;
    int tid = RD16;
    int size = RD16;
    CHKREG(dst);
    if (size) {
      OBJECT(L[dst],sbc->ty[tid],size);
    } else {
      L[dst] = MKIMM(sbc->ty[tid],0);
    }
    break;}
  case SBC_CNAS: {
    uint32_t expected = RD16;
    CHECK_NARGS(expected);
    break;}
  case SBC_ARGLIST: {
    int size = RD16;
    ARGLIST(size);
    for (int j = 0; j < size; j++) {
      int src = RD16;
      CHKREG(src);
      STARG(j,L[src]);
    }
    break;}
  case SBC_ARGLIST8: {
    int size = RD8;
    ARGLIST(size);
    for (int j = 0; j < size; j++) {
      int src = RD8;
      CHKREG(src);
      STARG(j,L[src]);
    }
    break;}
  case SBC_ARGLIST0: {
    ARGLIST(0);
    break;}
  case SBC_ARGLIST1: {
    int a;
    ARGLIST(1);
    a = RD8;
    CHKREG(a);
    STARG(0,L[a]);
    break;}
  case SBC_ARGLIST2: {
    int a;
    ARGLIST(2);
    a = RD8;
    CHKREG(a);
    STARG(0,L[a]);
    a = RD8;
    CHKREG(a);
    STARG(1,L[a]);
    break;}
  case SBC_ARGLIST3: {
    int a;
    ARGLIST(3);
    a = RD8;
    CHKREG(a);
    STARG(0,L[a]);
    a = RD8;
    CHKREG(a);
    STARG(1,L[a]);
    a = RD8;
    CHKREG(a);
    STARG(2,L[a]);
    break;}
  case SBC_ARGLIST4: {
    int a;
    ARGLIST(4);
    a = RD8;
    CHKREG(a);
    STARG(0,L[a]);
    a = RD8;
    CHKREG(a);
    STARG(1,L[a]);
    a = RD8;
    CHKREG(a);
    STARG(2,L[a]);
    a = RD8;
    CHKREG(a);
    STARG(3,L[a]);
    break;}
  case SBC_ARGLIST5: {
    int a;
    ARGLIST(5);
    a = RD8;
    CHKREG(a);
    STARG(0,L[a]);
    a = RD8;
    CHKREG(a);
    STARG(1,L[a]);
    a = RD8;
    CHKREG(a);
    STARG(2,L[a]);
    a = RD8;
    CHKREG(a);
    STARG(3,L[a]);
    a = RD8;
    CHKREG(a);
    STARG(4,L[a]);
    break;}
  case SBC_LIST: {
    uint32_t dst = RD16;
    uint32_t size = RD16;
    CHKREG(dst);
    LIST(L[dst],size);
    break;}
  case SBC_MOVEEMT: {
    int dst = RD16;
    CHKREG(dst);
    L[dst] = Empty;
    break;}
  case SBC_MOVENO: {
    int dst = RD16;
    CHKREG(dst);
    L[dst] = No;
    break;}
  case SBC_MOVETX: {
    int dst = RD16;
    int src = RD24;
    CHKREG(dst);
    L[dst] = sbc->tx[src];
    break;}
  case SBC_MOVETX8: {
    int dst = RD8;
    int src = RD8;
    CHKREG(dst);
    L[dst] = sbc->tx[src];
    break;}
  case SBC_MOVEMT: { //move method
    int dst = RD16;
    int src = RD24;
    CHKREG(dst);
    L[dst] = FXN(sbc->mt[src]);
    break;}
  case SBC_MOVEMT8: { //move method
    int dst = RD8;
    int src = RD8;
    CHKREG(dst);
    L[dst] = FXN(sbc->mt[src]);
    break;}
  case SBC_MOVEIM: { //move imported
    int dst = RD16;
    int src = RD24;
    CHKREG(dst);
    L[dst] = sbc->im[src];
    break;}
  case SBC_MOVE: { //move local to local
    int dst = RD16;
    int src = RD16;
    CHKREG(dst);
    CHKREG(src);
    L[dst] = L[src];
    break;}
  case SBC_MOVE8: { //move local to local
    int dst = RD8;
    int src = RD8;
    CHKREG(dst);
    CHKREG(src);
    L[dst] = L[src];
    break;}
  case SBC_MOVE4: { //move local to local
    int opr = RD8;
    int dst = opr&0xF;
    int src = opr>>4;
    CHKREG(dst);
    CHKREG(src);
    L[dst] = L[src];
    break;}
  case SBC_STOR: {
    int dst = RD16;
    int src = RD16;
    int index = RD16;
    CHKREG(dst);
    CHKREG(src);
    STOR(L[dst],index,L[src]);
    break;}
  case SBC_STOR8: {
    int dst = RD8;
    int src = RD8;
    int index = RD8;
    CHKREG(dst);
    CHKREG(src);
    STOR(L[dst],index,L[src]);
    break;}
  case SBC_ST4_0:
  case SBC_ST4_1:
  case SBC_ST4_2:
  case SBC_ST4_3:
  case SBC_ST4_4:
  case SBC_ST4_5:
  case SBC_ST4_6:
  case SBC_ST4_7:
  case SBC_ST4_8:
  case SBC_ST4_9:
  case SBC_ST4_A:
  case SBC_ST4_B:
  case SBC_ST4_C:
  case SBC_ST4_D:
  case SBC_ST4_E:
  case SBC_ST4_F: {
    int index = pin[-1]-SBC_ST4_0;
    int opr = RD8;
    int dst = opr&0xF;
    int src = opr>>4;
    CHKREG(dst);
    CHKREG(src);
    STOR(L[dst],index,L[src]);
    break;}
  case SBC_LOAD: {
    int dst = RD16;
    int src = RD16;
    int index = RD16;
    CHKREG(dst);
    CHKREG(src);
    LOAD(L[dst],L[src],index);
    break;}
  case SBC_LOAD8: {
    int dst = RD8;
    int src = RD8;
    int index = RD8;
    CHKREG(dst);
    CHKREG(src);
    LOAD(L[dst],L[src],index);
    break;}
  case SBC_LD4_0:
  case SBC_LD4_1:
  case SBC_LD4_2:
  case SBC_LD4_3:
  case SBC_LD4_4:
  case SBC_LD4_5:
  case SBC_LD4_6:
  case SBC_LD4_7:
  case SBC_LD4_8:
  case SBC_LD4_9:
  case SBC_LD4_A:
  case SBC_LD4_B:
  case SBC_LD4_C:
  case SBC_LD4_D:
  case SBC_LD4_E:
  case SBC_LD4_F: {
    int index = pin[-1]-SBC_LD4_0;
    int opr = RD8;
    int dst = opr&0xF;
    int src = opr>>4;
    CHKREG(dst);
    CHKREG(src);
    LOAD(L[dst],L[src],index);
    break;}
  case SBC_COPY: {
    int dst = RD16;
    int src = RD16;
    int dindex = RD16;
    int sindex = RD16;
    CHKREG(dst);
    CHKREG(src);
    COPY(L[dst],dindex,L[src],sindex);
    break;}
  case SBC_FXNB0: {
    int dst = RD8;
    CHKREG(dst);
    L[dst] = 0;
    break;}
  case SBC_FXNB8: {
    int dst = RD8;
    uint64_t imm = RD8;
    CHKREG(dst);
    L[dst] = FXN((int64_t)(int8_t)(uint8_t)imm);
    break;}
  case SBC_FXNB16: {
    int dst = RD8;
    uint64_t imm = RD16;
    CHKREG(dst);
    L[dst] = FXN((int64_t)(int16_t)(uint16_t)imm);
    break;}
  case SBC_FXNB32: {
    int dst = RD8;
    uint64_t imm = RD32;
    CHKREG(dst);
    L[dst] = FXN((int64_t)(int32_t)(uint32_t)imm);
    break;}
  case SBC_FXN0: {L[RD16] = 0; break;}
  case SBC_FXN8: {
    int dst = RD16;
    uint64_t imm = RD8;
    CHKREG(dst);
    L[dst] = FXN((int64_t)(int8_t)(uint8_t)imm);
    break;}
  case SBC_FXN16: {
    int dst = RD16;
    uint64_t imm = RD16;
    CHKREG(dst);
    L[dst] = FXN((int64_t)(int16_t)(uint16_t)imm);
    break;}
  case SBC_FXN32: {
    int dst = RD16;
    uint64_t imm = RD32;
    CHKREG(dst);
    L[dst] = FXN((int64_t)(int32_t)(uint32_t)imm);
    break;}
  case SBC_IMMB64: {
    int dst = RD8;
    CHKREG(dst);
#ifndef SBC_TRANSITION
    L[dst] = (dyn)RD64;
#else
    uint64_t imm = RD64;
    uint64_t tag = O_TAG(imm);
    if (tag == T_INT) {
      int64_t val = UNFXN(imm);
      L[dst] = FXN(val);
    } else if (tag == T_FLOAT) {
      float fval;
      STFLT(fval,imm);
      LDFLT(L[dst], fval);
    } else { // T_FIXTEXT
      char tmp[16];
      fixtext_decode(tmp, (dyn)imm);
      TEXT(L[dst], tmp);
    }
#endif
    break;}
  case SBC_IMM64: {
    int dst = RD16;
    CHKREG(dst);
#ifndef SBC_TRANSITION
    L[dst] = (dyn)RD64;
#else
    uint64_t imm = RD64;
    uint64_t tag = O_TAG(imm);
    if (tag == T_INT) {
      int64_t val = UNFXN(imm);
      L[dst] = FXN(val);
    } else if (tag == T_FLOAT) {
      float fval;
      STFLT(fval,imm);
      LDFLT(L[dst], fval);
    } else { // T_FIXTEXT
      char tmp[16];
      fixtext_decode(tmp, (dyn)imm);
      TEXT(L[dst], tmp);
    }
#endif
    break;}
  case SBC_FXT8: {
    int dst = RD8;
    uint64_t imm = RD8;
    CHKREG(dst);
    L[dst] = (dyn)FIXTEXT((uint64_t)imm);
    break;}
  case SBC_FXT16: {
    int dst = RD8;
    uint64_t imm = RD16;
    CHKREG(dst);
    L[dst] = (dyn)FIXTEXT((uint64_t)imm);
    break;}
  case SBC_FXT24: {
    int dst = RD8;
    uint64_t imm = RD24;
    CHKREG(dst);
    L[dst] = (dyn)FIXTEXT((uint64_t)imm);
    break;}
  case SBC_FXT32: {
    int dst = RD8;
    uint64_t imm = RD32;
    CHKREG(dst);
    L[dst] = (dyn)FIXTEXT((uint64_t)imm);
    break;}
  case SBC_FXT40: {
    int dst = RD8;
    uint64_t imm = RD40;
    CHKREG(dst);
    L[dst] = (dyn)FIXTEXT((uint64_t)imm);
    break;}
  case SBC_FXT48: {
    int dst = RD8;
    uint64_t imm = RD48;
    CHKREG(dst);
    L[dst] = (dyn)FIXTEXT((uint64_t)imm);
    break;}
  case SBC_FXT56: {
    int dst = RD8;
    uint64_t imm = RD56;
    CHKREG(dst);
    L[dst] = (dyn)FIXTEXT((uint64_t)imm);
    break;}
  case SBC_FXNTAG: {
    int dst = RD16;
    int src = RD16;
    CHKREG(dst);
    CHKREG(src);
    L[dst] = FXN(O_TAG(L[src]));
    break;}
  case SBC_FXNLISTN: {
    int dst = RD16;
    int src = RD16;
    CHKREG(dst); CHKREG(src);
    FXNLISTN(L[dst],L[src]);
    break;}
  case SBC_FXNLGET: {
    int dst = RD16;
    int src = RD16;
    int index = RD16;
    CHKREG(dst); CHKREG(src); CHKREG(index);
    dyn ss = L[src];
    dyn ii = L[index];
    if (TAGIS(T_LIST,ss) && TAGIS(T_INT,ii)
       && (uint64_t)ii < (uint64_t)FXN(LIST_SIZE(ss))) {
      FXNLGET(L[dst],ss,ii);
      MCACHE_SKIP;
    } else {
      ARGLIST2(L[src],L[index]);
      MCACHE_CALL(L[dst],L[src],m_get);
    }
    break;}
  case SBC_FXNLSET: {
    int dst = RD16;
    int src = RD16;
    int index = RD16;
    int val = RD16;
    CHKREG(dst); CHKREG(src); CHKREG(index); CHKREG(val);
    dyn ss = L[src];
    dyn ii = L[index];
    if (TAGIS(T_LIST,ss) && TAGIS(T_INT,ii)
       && (uint64_t)ii < (uint64_t)FXN(LIST_SIZE(ss))) {
      FXNLSET(L[dst],ss,ii,L[val]);
      MCACHE_SKIP;
    } else {
      ARGLIST3(L[src],L[index],L[val]);
      MCACHE_CALL(L[dst],L[src],m_set);
    }
    break;}
  case SBC_FXNLSETIR: {
    dyn dummy;
    int src = RD16;
    int index = RD16;
    int val = RD16;
    CHKREG(src); CHKREG(index); CHKREG(val);
    dyn ss = L[src];
    dyn ii = L[index];
    if (TAGIS(T_LIST,ss) && TAGIS(T_INT,ii)
       && (uint64_t)ii < (uint64_t)FXN(LIST_SIZE(ss))) {
      FXNLSET(dummy,L[src],L[index],L[val]);
      MCACHE_SKIP;
    } else {
      ARGLIST3(L[src],L[index],L[val]);
      MCACHE_CALL(dummy,L[src],m_set);
    }
    break;}
  case SBC_FXNSIZE: {
    int dst = RD16;
    int src = RD16;
    CHKREG(dst);
    CHKREG(src);
    L[dst] = FXN(LIST_SIZE(L[src]));
    break;}
  case SBC_ABS: {
    int dst = RD16; int a = RD16; 
    CHKREG(dst); CHKREG(a);
    dyn aa = L[a];
    if (TAGIS(T_INT, aa)) {
      int64_t val = UNFXN(aa);
      if (val < 0) aa = FXN(-val);
      L[dst] = aa;
    } else if (TAGIS(T_FLOAT, aa)) {
      float fa, fb;
      STFLT(fa,aa);
      if (fa < 0.0f) LDFLT(aa,-fa);
      L[dst] = aa;
    } else {
      ARGLIST1(L[a]);
      MCALL(L[dst],L[a],m_abs);
    }
    break;}
  case SBC_NEG: {
    int dst = RD16; int a = RD16; 
    CHKREG(dst); CHKREG(a);
    dyn aa = L[a];
    if (TAGIS(T_INT, aa)) {
      FXNNEG(L[dst],L[a]);
    } else {
      ARGLIST1(L[a]); //cant use aa and bb here, cuz GC could have run
      MCALL(L[dst],L[a],m_neg);
    }
    break;}
  case SBC_FXNADD: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    dyn aa = L[a];
    if (TAGIS(T_INT, aa)) {
      dyn bb = L[b];
      if (TAGIS(T_INT, bb)) FXNADD(L[dst],aa,bb);
      else {
        float fa, fb;
        fa = (float)UNFXN(aa);
        STFLT(fb,bb);
        LDFLT(L[dst],fa+fb);
      }
    } else {
      ARGLIST2(L[a],L[b]); //cant use aa and bb here, cuz GC could have run
      MCALL(L[dst],L[a],m_add);
    }
    break;}
  case SBC_FXNSUB: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    dyn aa = L[a];
    if (TAGIS(T_INT, aa)) {
      dyn bb = L[b];
      if (TAGIS(T_INT, bb)) FXNSUB(L[dst],aa,bb);
      else {
        float fa, fb;
        fa = (float)UNFXN(aa);
        STFLT(fb,bb);
        LDFLT(L[dst],fa-fb);
      }
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_sub);
    }
    break;}
  case SBC_FXNMUL: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    dyn aa = L[a];
    if (TAGIS(T_INT, aa)) {
      dyn bb = L[b];
      if (TAGIS(T_INT, bb)) FXNMUL(L[dst],aa,bb);
      else {
        float fa, fb;
        fa = (float)UNFXN(aa);
        STFLT(fb,bb);
        LDFLT(L[dst],fa*fb);
      }
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_mul);
    }
    break;}
  case SBC_FXNDIV: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    dyn aa = L[a];
    if (TAGIS(T_INT, aa)) {
      dyn bb = L[b];
      if (TAGIS(T_INT, bb)) FXNDIV(L[dst],aa,bb);
      else {
        float fa, fb;
        fa = (float)UNFXN(aa);
        STFLT(fb,bb);
        if (fb == 0.0) fb = FLT_MIN;
        LDFLT(L[dst],fa/fb);
      }
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_div);
    }
    break;}
  case SBC_FXNREM: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    FXNREM(L[dst],L[a],L[b]);
    dyn aa = L[a];
    if (TAGIS(T_INT, aa)) {
      dyn bb = L[b];
      if (TAGIS(T_INT, bb)) FXNREM(L[dst],aa,bb);
      else {
        float fa, fb, r;
        fa = (float)UNFXN(aa);
        STFLT(fb,bb);
        if (fb == 0.0) fb = FLT_MIN;
        r = fa/fb;
        LDFLT(L[dst],(r-(float)(int64_t)r)*fb);
      }
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_rem);
    }
    break;}
  case SBC_IMMEQ: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    if (TAGIS(T_INT, L[a])) {
      IMMEQ(L[dst],L[a],L[b]);
      MCACHE_SKIP;
    } else {
      ARGLIST2(L[a],L[b]);
      MCACHE_CALL(L[dst],L[a],m_eq);
    }
    break;}
  case SBC_IMMNE: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    if (TAGIS(T_INT, L[a])) {
      IMMNE(L[dst],L[a],L[b]);
      MCACHE_SKIP;
    } else {
      ARGLIST2(L[a],L[b]);
      MCACHE_CALL(L[dst],L[a],m_ne);
    }
    break;}
  case SBC_FXNLT: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    if (TAGIS(T_INT, L[a]) && TAGIS(T_INT, L[b])) {
      FXNLT(L[dst],L[a],L[b]);
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_lt);
    }
    break;}
  case SBC_FXNGT: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    if (TAGIS(T_INT, L[a]) && TAGIS(T_INT, L[b])) {
      FXNGT(L[dst],L[a],L[b]);
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_gt);
    }
    break;}
  case SBC_FXNLTE: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    if (TAGIS(T_INT, L[a]) && TAGIS(T_INT, L[b])) {
      FXNLTE(L[dst],L[a],L[b]);
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_lte);
    }
    break;}
  case SBC_FXNGTE: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    if (TAGIS(T_INT, L[a]) && TAGIS(T_INT, L[b])) {
      FXNGTE(L[dst],L[a],L[b]);
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_gte);
    }
    break;}
  case SBC_FXNAND: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    if (TAGIS(T_INT, L[a]) && TAGIS(T_INT, L[b])) {
      FXNAND(L[dst],L[a],L[b]);
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_and);
    }
    break;}
  case SBC_FXNIOR: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    if (TAGIS(T_INT, L[a]) && TAGIS(T_INT, L[b])) {
      FXNIOR(L[dst],L[a],L[b]);
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_ior);
    }
    break;}
  case SBC_FXNXOR: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    if (TAGIS(T_INT, L[a]) && TAGIS(T_INT, L[b])) {
      FXNXOR(L[dst],L[a],L[b]);
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_xor);
    }
    break;}
  case SBC_FXNSHL: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    if (TAGIS(T_INT, L[a]) && TAGIS(T_INT, L[b])) {
      FXNSHL(L[dst],L[a],L[b]);
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_shl);
    }
    break;}
  case SBC_FXNSHR: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    if (TAGIS(T_INT, L[a]) && TAGIS(T_INT, L[b])) {
      FXNSHR(L[dst],L[a],L[b]);
    } else {
      ARGLIST2(L[a],L[b]);
      MCALL(L[dst],L[a],m_shr);
    }
    break;}
  case SBC_SAME: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    IMMEQ(L[dst],L[a],L[b]);
    break;}
  case SBC_VARY: {
    int dst = RD16; int a = RD16; int b = RD16;
    CHKREG(dst); CHKREG(a); CHKREG(b);
    IMMNE(L[dst],L[a],L[b]);
    break;}
  case SBC_TINIT: {
    int type = RD16;
    int size = RD16;
    int name = RD24;
    set_type_size_and_name((int64_t)sbc->ty[type],size,sbc->tx[name]);
    break;}
  case SBC_TINITI: {
    int tag = RD16;
    int size = RD16;
    dyn name = (dyn)RD64; //FIXTEXT READ
    set_type_size_and_name((int64_t)sbc->ty[tag],size,name);
    break;}
  case SBC_SUBTYPE: {
    int super = RD16;
    int sub = RD16;
    add_subtype((int)(int64_t)sbc->ty[super],(int)(int64_t)sbc->ty[sub]);
    break;}
  case SBC_DMET: {
    int tyidx = RD16;
    int mtidx = RD24;
    int handler = RD16;
    add_method((int)(int64_t)sbc->ty[tyidx], sbc->mt[mtidx], L[handler]);
    break;}
  case SBC_INC: {
    int dst = RD16; int a = RD16; 
    CHKREG(dst); CHKREG(a);
    dyn aa = L[a];
    if (TAGIS(T_INT, aa)) {
      FXNADD(L[dst],aa,FXN(1));
    } else {
      ARGLIST1(L[a]); //cant use aa and bb here, cuz GC could have run
      MCALL(L[dst],L[a],m_inc);
    }
    break;}
  case SBC_DEC: {
    int dst = RD16; int a = RD16; 
    CHKREG(dst); CHKREG(a);
    dyn aa = L[a];
    if (TAGIS(T_INT, aa)) {
      FXNSUB(L[dst],aa,FXN(1));
    } else {
      ARGLIST1(L[a]); //cant use aa and bb here, cuz GC could have run
      MCALL(L[dst],L[a],m_dec);
    }
    break;}
  case SBC_NOT: {
    int dst = RD16;
    int src = RD16;
    CHKREG(dst);
    CHKREG(src);
    L[dst] = L[src] ? FXN(0) : FXN(1);
    break;}
  case SBC_GOT: {
    int dst = RD16;
    int src = RD16;
    CHKREG(dst);
    CHKREG(src);
    L[dst] = L[src]!=No ? FXN(1) : FXN(0);
    break;}
  case SBC_NO: {
    int dst = RD16;
    int src = RD16;
    CHKREG(dst);
    CHKREG(src);
    L[dst] = L[src]==No ? FXN(1) : FXN(0);
    break;}
  case SBC_GID: {
    int dst = RD16;
    int src = RD16;
    CHKREG(dst);
    CHKREG(src);
    L[dst] = STRIP_TAG(L[src]);
    break;}
  case SBC_CURMET: {
    int dst = RD16;
    CHKREG(dst);
    THIS_METHOD(L[dst]);
    break;}
  case SBC_MNAME: {
    int dst = RD16;
    int src = RD16;
    CHKREG(dst);
    CHKREG(src);
    L[dst] = get_method_name(UNFXN(L[src]));
    break;}
  case SBC_FATAL: {
    int msg = RD16;
    CHKREG(msg);
    FATAL(L[msg]);
    break;}
  case SBC_GC0: { GC_DISABLE(); break;}
  case SBC_GC1: { GC_ENABLE(); break;}
  case SBC_NFI_INT: {
    int r = RD16;
    CHKREG(r);
    dyn src = L[r];
    FFI_ARG_int(tmp, src);
    *(int*)api.nfi_parg = tmp;
    ++api.nfi_parg;
    break;}
  case SBC_NFI_PTR: {
    int r = RD16;
    CHKREG(r);
    dyn src = L[r];
    FFI_ARG_ptr(tmp, src);
    *(void**)api.nfi_parg = tmp;
    ++api.nfi_parg;
    break;}
  case SBC_NFI_TXT: {
    int r = RD16;
    CHKREG(r);
    dyn src = L[r];
    FFI_ARG_text(tmp, src);
    *(char**)api.nfi_parg = tmp;
    ++api.nfi_parg;
    break;}
  case SBC_NFI_U4: {
    int r = RD16;
    CHKREG(r);
    dyn src = L[r];
    FFI_ARG_u4(tmp, src);
    *(uint32_t*)api.nfi_parg = tmp;
    ++api.nfi_parg;
    break;}
  case SBC_NFI_FLT: {
    int r = RD16;
    CHKREG(r);
    dyn src = L[r];
    FFI_ARG_float(tmp, src);
    *(float*)api.nfi_parg = tmp;
    ++api.nfi_parg;
    break;}
  case SBC_NFI_DBL: {
    int r = RD16;
    CHKREG(r);
    dyn src = L[r];
    FFI_ARG_double(tmp, src);
    *(double*)api.nfi_parg = tmp;
    ++api.nfi_parg;
    break;}
  case SBC_NFI: {
    int pfn = RD16; //native function pointer
    int tri = RD16; //trampoline index
    CHKREG(pfn);
    void *target = NFI_DECPTR(L[pfn]);
    /* sffi_call returns the raw bit-pattern of the return register
     * (rax for int/ptr, xmm0 for float/double). The switch below
     * reinterprets per the declared return type. */
    uint64_t retval = sffi_call(
        (const sffi_sig_t *)sbc->nfi_trmps[tri],
        target, api.nfi_args);
    api.nfi_parg = api.nfi_args;
    int rtid = RD8;
    int dst = RD16;
    CHKREG(dst);
    if (dst) switch (rtid) {
    case SBC_NFI_VOID: L[dst] = No;
    case SBC_NFI_INT: {
      FFI_FROM_int(L[dst],*(int*)&retval);
      break;}
    case SBC_NFI_PTR: {
      FFI_FROM_ptr(L[dst],*(void**)&retval);
      break;}
    case SBC_NFI_TXT: {
      FFI_FROM_text(L[dst],*(char**)&retval);
      break;}
    case SBC_NFI_U4: {
      FFI_FROM_u4(L[dst],*(uint32_t*)&retval);
      break;}
    case SBC_NFI_FLT: {
      FFI_FROM_float(L[dst],*(float*)&retval);
      break;}
    case SBC_NFI_DBL: {
      FFI_FROM_double(L[dst],*(double*)&retval);
      break;}
    }
    break;}
  case SBC_NLDU1: {
    int dst = RD16; int ptr = RD16; int ofs = RD16;
    CHKREG(dst); CHKREG(ptr); CHKREG(ofs);
    FFI_GET(L[dst],uint8_t,L[ptr],L[ofs]);
    break;}
  case SBC_NLDU2: {
    int dst = RD16; int ptr = RD16; int ofs = RD16;
    CHKREG(dst); CHKREG(ptr); CHKREG(ofs);
    FFI_GET(L[dst],uint16_t,L[ptr],L[ofs]);
    break;}
  case SBC_NLDU4: {
    int dst = RD16; int ptr = RD16; int ofs = RD16;
    CHKREG(dst); CHKREG(ptr); CHKREG(ofs);
    FFI_GET(L[dst],uint32_t,L[ptr],L[ofs]);
    break;}
  case SBC_NLDS1: {
    int dst = RD16; int ptr = RD16; int ofs = RD16;
    CHKREG(dst); CHKREG(ptr); CHKREG(ofs);
    FFI_GET(L[dst],int8_t,L[ptr],L[ofs]);
    break;}
  case SBC_NLDS2: {
    int dst = RD16; int ptr = RD16; int ofs = RD16;
    CHKREG(dst); CHKREG(ptr); CHKREG(ofs);
    FFI_GET(L[dst],int16_t,L[ptr],L[ofs]);
    break;}
  case SBC_NLDS4: {
    int dst = RD16; int ptr = RD16; int ofs = RD16;
    CHKREG(dst); CHKREG(ptr); CHKREG(ofs);
    FFI_GET(L[dst],int32_t,L[ptr],L[ofs]);
    break;}
  case SBC_NSTU1: {
    int ptr = RD16; int ofs = RD16; int val = RD16;
    CHKREG(val); CHKREG(ptr); CHKREG(ofs);
    FFI_SET(uint8_t,L[ptr],L[ofs],L[val]);
    break;}
  case SBC_NSTU2: {
    int ptr = RD16; int ofs = RD16; int val = RD16;
    CHKREG(val); CHKREG(ptr); CHKREG(ofs);
    FFI_SET(uint16_t,L[ptr],L[ofs],L[val]);
    break;}
  case SBC_NSTU4: {
    int ptr = RD16; int ofs = RD16; int val = RD16;
    CHKREG(val); CHKREG(ptr); CHKREG(ofs);
    FFI_SET(uint32_t,L[ptr],L[ofs],L[val]);
    break;}
  case SBC_NSTS1: {
    int ptr = RD16; int ofs = RD16; int val = RD16;
    CHKREG(val); CHKREG(ptr); CHKREG(ofs);
    FFI_SET(int8_t,L[ptr],L[ofs],L[val]);
    break;}
  case SBC_NSTS2: {
    int ptr = RD16; int ofs = RD16; int val = RD16;
    CHKREG(val); CHKREG(ptr); CHKREG(ofs);
    FFI_SET(int16_t,L[ptr],L[ofs],L[val]);
    break;}
  case SBC_NSTS4: {
    int ptr = RD16; int ofs = RD16; int val = RD16;
    CHKREG(val); CHKREG(ptr); CHKREG(ofs);
    FFI_SET(int32_t,L[ptr],L[ofs],L[val]);
    break;}
#define CTX_BTLAND 0
#define CTX_BTJUMP 1
  case SBC_CTX: {
    int type = RD8;
    if (type == CTX_BTLAND) {
      int dst = RD16;
      CHKREG(dst);
      int entry;

      jmp_state * volatile js_; //volatile to prevent compiler from caching it
      GC_DISABLE();
      api.jmp_return = alloc_bytes(sizeof(jmp_state));
      GC_ENABLE();
      js_ = (jmp_state*)BYTES_DATA(api.jmp_return);
      //js_->uwh = (dyn)123;
      js_->uwh = api.puwh;
      js_->frame = api.frame;
      js_->ip = pin;
      jmp_buf jb;
      entry = setjmp(jb);
      js_->anchor = &jb;
      entry = !entry;
      if (!entry) {
        js_ = (jmp_state*)BYTES_DATA(LGET(api.jmp_return,0));
        api.jmp_return = LGET(api.jmp_return,1);
      }
      pin = js_->ip;

      dyn *r;
      GC_DISABLE();
      LIST(r,2);
      LSET(r,0,FXN(entry));
      LSET(r,1,api.jmp_return);
      GC_ENABLE();
      L[dst] = r;
      api.jmp_return = 0;
    } else if (type == CTX_BTJUMP) {
      int state = RD16;
      int value = RD16;
      CHKREG(state);
      CHKREG(value);
      btjump(L[state],L[value]);
    } else {
      fprintf(stderr, "SBC_CTX: bad type=%d\n", type);
    }
    break;}
  default: {
#if 1
    int opcode = pin[-1];
    fprintf(stderr, "sbc_exec_fn: bad opcode=%x\n", opcode);
#else
    char *head = 0;
    int oi = opcode;
    while (!head) {
      head = hmget(sbcasm,oi);
      oi--;
    }
    char *name = head ? head : "unk";
    fprintf(stderr, "sbc_exec_fn: bad opcode=%x (%s)\n", opcode, name);
#endif
    exit(-1);
    break;}
  }}
  return (dyn)0;
}

dyn sbc_exec(sbc_t *sbc) {
  sbc_prepare(sbc);

  void *entry = (void*)sbc->hooks[0];
  OPEN_FRAME(0);
  PROLOGUE(FRAME_HEADER_SIZE);

  dyn centry=0;
  dyn r;

  GC_DISABLE();
  CLOSURE(centry,entry,0);
  GC_ENABLE();

  CALL(r,centry); // run the module toplevel

  CLOSE_FRAME;

  return r;
}

dyn sbc_exec_file(char *path) {
  sbc_t *sbc = sbc_load(path);
  if (!sbc) fatal("Couldnt load %s\n", path);
  return sbc_exec(sbc);
}


