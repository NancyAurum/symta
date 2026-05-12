





#if 0
NOINLINE void target1(int arg) {
  static volatile int val;
  val = arg;
}
#endif

NOINLINE void target2(int arg, int arg2) {
  static volatile int val;
  val = arg;
  val = arg2;
}

void target(void *caller, void *ptr) {
  //target2(7,3);
  fprintf(stderr, "hello: %p, %p\n", ptr, caller);

}

//basically curries the 2nd argument of a 2-argument function
uint8_t *emit_curry_hook(uint8_t *dst, void *target, void *payload) {
#undef EMIT8
#define EMIT8(x) do {\
  uint32_t x1 = (uint32_t)(x); *wb++ = (x1)&0xFF; } while(0)

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
  EMIT8(0xE8); //push ip
  EMIT32(0);
  EMIT8(0x5F);  //pop rdi
#endif

  EMIT8(0xFF);  //jmp rax
  EMIT8(0xE0);

  return wb;
}



#include <cinvoke.h>

NOINLINE void target1(int arg) {
  fprintf(stderr, "target1: %d\n", arg);
}

NOINLINE void target2(int arg, int arg2) {
  fprintf(stderr, "target2: %d, %d\n", arg, arg2);
}

NOINLINE double target3(int arg, int arg2, int arg3) {
  fprintf(stderr, "target3: %d, %d, %d\n", arg, arg2, arg3);
  return 123.456;
}

void test_cinvoke() {
  CInvContext *ctx = cinv_context_create();

  int a1 = 123, a2 = 456, a3 = 789;
  void *args1[] = {&a1};
  void *args2[] = {&a1, &a2};
  void *args3[] = {&a1, &a2, &a3};
  void *retval;
  double fretval;

  CInvFunction *proto1 = cinv_function_create(ctx, CINV_CC_DEFAULT, "", "i");
  CInvFunction *proto2 = cinv_function_create(ctx, CINV_CC_DEFAULT, "", "ii");
  CInvFunction *proto3 = cinv_function_create(ctx, CINV_CC_DEFAULT, "d", "iii");

  cinv_function_invoke(ctx, proto1, &target1, &retval, args1);
  cinv_function_invoke(ctx, proto2, &target2, &retval, args2);
  cinv_function_invoke(ctx, proto3, &target3, &fretval, args3);
  
  fprintf(stderr, "ret=%f\n", fretval);

  cinv_function_delete(ctx, proto1);
  cinv_function_delete(ctx, proto2);
  cinv_function_delete(ctx, proto3);

  cinv_context_delete(ctx);
  exit(-1);
}


uint8_t buf[128];
void test_curry() {
  emit_curry_hook(buf, &target, buf);
  make_executable(buf, 128);
  ((void (*)(void *p))buf)(&test_curry);
  fprintf(stderr, "world: %p, %p\n", buf, &test_curry);
  int sz = buf[0];
  int as[sz];
  exit(-1);
}










void sif_test() {
  char *src = "/Users/macbook/Documents/git/symta/build/symta/lib/core_.txt";
  char *dst = "/Users/macbook/Documents/git/symta/build/symta/lib/test";
  int64_t file_size;
  char *file = fs_get(&file_size, src);
  if (!file) {
    fprintf(stderr, "couldnt open sif file `%s`\n", src);
    exit(-1);
  }
  char *result = sif_to_native(dst, file, file_size);
  if (result) {
    fprintf(stderr, "sif_test: %s\n", result);
  }
  free(file);
  exit(-1);
}


