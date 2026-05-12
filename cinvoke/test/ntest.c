#include <stdio.h>
#include <stdlib.h>
#include <cinvoke.h>

#ifdef ARCH_GCC_X64_WIN
#include "../../../runtime/w64/dlfcn.c"
#include "../../../runtime/w64/mman.c"
#endif

#define NOINLINE

#define TEST_FLT

#ifdef TEST_FLT

NOINLINE void target0() {
  fprintf(stderr, "target0\n");
}

NOINLINE void target1(float arg1) {
  fprintf(stderr, "target1: %f\n", arg1);
}

NOINLINE void target2(float arg1, float arg2) {
  fprintf(stderr, "target2: %f, %f\n", arg1, arg2);
}

NOINLINE void target3(float arg1, float arg2, float arg3) {
  fprintf(stderr, "target3: %f, %f, %f\n", arg1, arg2, arg3);
}

NOINLINE void target4(float arg1, float arg2, float arg3, float arg4) {
  fprintf(stderr, "target4: %f, %f, %f, %f\n", arg1, arg2, arg3, arg4);
}

NOINLINE void target5(float arg1, float arg2, float arg3, float arg4
           ,float arg5) {
  fprintf(stderr, "target5: %f, %f, %f, %f, %f\n"
         , arg1, arg2, arg3, arg4, arg5);
}

NOINLINE void target6(float arg1, float arg2, float arg3, float arg4
           ,float arg5, float arg6) {
  fprintf(stderr, "target6: %f, %f, %f, %f, %f, %f\n"
         , arg1, arg2, arg3, arg4, arg5, arg6);
}

NOINLINE void target7(float arg1, float arg2, float arg3, float arg4
           ,float arg5, float arg6, float arg7) {
  fprintf(stderr, "target7: %f, %f, %f, %f, %f, %f, %f\n"
         , arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}


NOINLINE void target8(float arg1, float arg2, float arg3, float arg4
           ,float arg5, float arg6, float arg7, float arg8) {
  fprintf(stderr, "target8: %f, %f, %f, %f, %f, %f, %f, %f\n"
         , arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}

NOINLINE void target9(float arg1, float arg2, float arg3, float arg4
           ,float arg5, float arg6, float arg7, float arg8, float arg9) {
  fprintf(stderr, "target9: %f, %f, %f, %f, %f, %f, %f, %f, %f\n"
         , arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}

NOINLINE void target10(float arg1, float arg2, float arg3, float arg4
       ,float arg5, float arg6, float arg7, float arg8, float arg9
       , float arg10) {
  fprintf(stderr, "target10: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n"
         , arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
}


NOINLINE void target11(float arg1, float arg2, float arg3, float arg4
       ,float arg5, float arg6, float arg7, float arg8, float arg9
       , float arg10, float arg11) {
  fprintf(stderr, "target10: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n"
         , arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
}



#define CI_MAX_ARGS 32

void test_cinvoke() {
  float args[CI_MAX_ARGS];
  void *pargs[CI_MAX_ARGS];
  void *pfns[CI_MAX_ARGS];
  void *retval;

  CInvContext *ctx = cinv_context_create();

  for (int i = 0; i < CI_MAX_ARGS; i++) {
    float v = (float)(i+1);
    args[i] = (float)v + (float)v/10;
    pargs[i] = &args[i];
  }
  pfns[0] = &target0;
  pfns[1] = &target1;
  pfns[2] = &target2;
  pfns[3] = &target3;
  pfns[4] = &target4;
  pfns[5] = &target5;
  pfns[6] = &target6;
  pfns[7] = &target7;
  pfns[8] = &target8;
  pfns[9] = &target9;
  pfns[10] = &target10;
  pfns[11] = &target11;
  pfns[12] = 0;

  for (int i = 0; pfns[i]; i++) {
    char type[CI_MAX_ARGS+1];
    
    for (int j = 0; j < i; j++) {
      type[j] = 'f';
    }
    type[i] = 0;

    CInvFunction *trmpl = cinv_function_create(ctx, CINV_CC_DEFAULT, "", type);
    cinv_function_invoke(ctx, trmpl, pfns[i], &retval, pargs);
    cinv_function_delete(ctx, trmpl);
  }

  cinv_context_delete(ctx);
  exit(-1);
}

#else

NOINLINE void target0() {
  fprintf(stderr, "target0\n");
}

NOINLINE void target1(int arg1) {
  fprintf(stderr, "target1: %d\n", arg1);
}

NOINLINE void target2(int arg1, int arg2) {
  fprintf(stderr, "target2: %d, %d\n", arg1, arg2);
}

NOINLINE void target3(int arg1, int arg2, int arg3) {
  fprintf(stderr, "target3: %d, %d, %d\n", arg1, arg2, arg3);
}

NOINLINE void target4(int arg1, int arg2, int arg3, int arg4) {
  fprintf(stderr, "target4: %d, %d, %d, %d\n", arg1, arg2, arg3, arg4);
}

NOINLINE void target5(int arg1, int arg2, int arg3, int arg4
           ,int arg5) {
  fprintf(stderr, "target5: %d, %d, %d, %d, %d\n"
         , arg1, arg2, arg3, arg4, arg5);
}

NOINLINE void target6(int arg1, int arg2, int arg3, int arg4
           ,int arg5, int arg6) {
  fprintf(stderr, "target6: %d, %d, %d, %d, %d, %d\n"
         , arg1, arg2, arg3, arg4, arg5, arg6);
}

NOINLINE void target7(int arg1, int arg2, int arg3, int arg4
           ,int arg5, int arg6, int arg7) {
  fprintf(stderr, "target7: %d, %d, %d, %d, %d, %d, %d\n"
         , arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}


NOINLINE void target8(int arg1, int arg2, int arg3, int arg4
           ,int arg5, int arg6, int arg7, int arg8) {
  fprintf(stderr, "target8: %d, %d, %d, %d, %d, %d, %d, %d\n"
         , arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
}

NOINLINE void target9(int arg1, int arg2, int arg3, int arg4
           ,int arg5, int arg6, int arg7, int arg8, int arg9) {
  fprintf(stderr, "target9: %d, %d, %d, %d, %d, %d, %d, %d, %d\n"
         , arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}

NOINLINE void target10(int arg1, int arg2, int arg3, int arg4, int arg5
                      ,int arg6, int arg7, int arg8, int arg9, int arg10) {
  fprintf(stderr, "target10: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n"
         , arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
}

#define CI_MAX_ARGS 32

void test_cinvoke() {
  int args[CI_MAX_ARGS];
  void *pargs[CI_MAX_ARGS];
  void *pfns[CI_MAX_ARGS];
  void *retval;

  CInvContext *ctx = cinv_context_create();

  for (int i = 0; i < CI_MAX_ARGS; i++) {
    int v = i+1;
    args[i] = v;
    pargs[i] = &args[i];
  }
  pfns[0] = &target0;
  pfns[1] = &target1;
  pfns[2] = &target2;
  pfns[3] = &target3;
  pfns[4] = &target4;
  pfns[5] = &target5;
  pfns[6] = &target6;
  pfns[7] = &target7;
  pfns[8] = &target8;
  pfns[9] = &target9;
  pfns[10] = &target10;
  pfns[11] = 0; 

  for (int i = 0; pfns[i]; i++) {
    char type[CI_MAX_ARGS+1];

    for (int j = 0; j < i; j++) {
      type[j] = 'i';
    }
    type[i] = 0;

    //fprintf(stderr, "%s\n",type);
    CInvFunction *trmpl = cinv_function_create(ctx, CINV_CC_DEFAULT, "", type);
    cinv_function_invoke(ctx, trmpl, pfns[i], &retval, pargs);
    cinv_function_delete(ctx, trmpl);
  }

  cinv_context_delete(ctx);
  exit(-1);
}
#endif

#if 0
NOINLINE void test_fn(float arg1, float arg2, float arg3, float arg4
           ,float arg5, float arg6, int arg7) {
  fprintf(stderr, "target7: %f, %f, %f, %f, %f, %f, %d\n"
         , arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}


void test_cinvoke2() {
  float arg1 = 1.1;
  float arg2 = 2.2;
  float arg3 = 3.3;
  float arg4 = 4.4;
  float arg5 = 5.5;
  float arg6 = 6.6;
  int arg7 = 7;
  void *pargs[7];
  void *retval;
  char *type = "ffffffi";

  CInvContext *ctx = cinv_context_create();

  pargs[0] = &arg1;
  pargs[1] = &arg2;
  pargs[2] = &arg3;
  pargs[3] = &arg4;
  pargs[4] = &arg5;
  pargs[5] = &arg6;
  pargs[6] = &arg7;

  CInvFunction *trmpl = cinv_function_create(ctx, CINV_CC_DEFAULT, "", type);
  cinv_function_invoke(ctx, trmpl, test_fn, &retval, pargs);
  cinv_function_delete(ctx, trmpl);

  cinv_context_delete(ctx);
  exit(-1);
}
#else


NOINLINE void test_fn(float arg1, int arg2, float arg3, int arg4) {
  fprintf(stderr, "target7: %f, %d, %f, %d\n"
         , arg1, arg2, arg3, arg4);
}


void test_cinvoke2() {
  float arg1 = 1.1;
  int arg2 = 2;
  float arg3 = 3;
  int arg4 = 444;
  void *pargs[7];
  void *retval;
  char *type = "fifi";

  CInvContext *ctx = cinv_context_create();

  pargs[0] = &arg1;
  pargs[1] = &arg2;
  pargs[2] = &arg3;
  pargs[3] = &arg4;

  CInvFunction *trmpl = cinv_function_create(ctx, CINV_CC_DEFAULT, "", type);
  cinv_function_invoke(ctx, trmpl, test_fn, &retval, pargs);
  cinv_function_delete(ctx, trmpl);

  cinv_context_delete(ctx);
  exit(-1);
}
#endif

int main(int argc, char **argv) {
  test_cinvoke();
  return 0;
}

