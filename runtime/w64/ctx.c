#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <windows.h>

#include "ctx.h"

#if 0
void ctx_dump(void *ctx) {
  CONTEXT *context = (CONTEXT*)ctx;
  int i;

  printf("ACDBx : %016I64x %016I64x %016I64x %016I64x\n",
         c->Rax, c->Rcx, c->Rdx, c->Rbx);
  printf("SBpSDi: %016I64x %016I64x %016I64x %016I64x\n",
         c->Rsp, c->Rbp, c->Rsi, c->Rdi);
  printf("r8-11 : %016I64x %016I64x %016I64x %016I64x\n",
         c->R8,  c->R9,  c->R10, c->R11);
  printf("r12-15: %016I64x %016I64x %016I64x %016I64x\n",
         c->R12, c->R13, c->R14, c->R15);

  for (i = 0; i < 16; i += 2)
    printf("x%02d-%02d: %016I64x.%016I64x %016I64x.%016I64x\n",
	   i, i+1,
           c->FloatSave.XmmRegisters[i].High,
           c->FloatSave.XmmRegisters[i].Low,
           c->FloatSave.XmmRegisters[i + 1].High,
           c->FloatSave.XmmRegisters[i + 1].Low);

  fflush (stdout);
}
#endif

void *ctx_function_at(void *ip) {
  ULONG64 ControlPC;
  ULONG64 ImageBase;
  ControlPC = (ULONG64)ip;
  PRUNTIME_FUNCTION entry = RtlLookupFunctionEntry(ControlPC, &ImageBase, NULL);
  return (void*)((ULONG64)entry->BeginAddress + ImageBase);
}

void *ctx_module_at(void *ip) {
  ULONG64 ControlPC;
  ULONG64 ImageBase;
  ControlPC = (ULONG64)ip;
  PRUNTIME_FUNCTION entry = RtlLookupFunctionEntry(ControlPC, &ImageBase, NULL);
  return (void*)ImageBase;
}

void *ctx_unwind(void *ctx) {
  PRUNTIME_FUNCTION entry;
  ULONG64 ControlPC;
  ULONG64 ImageBase;
  PVOID HandlerData;
  ULONG64 EstablisherFrame;
  CONTEXT *context = (CONTEXT*)ctx;
  ControlPC = context->Rip;
  entry = RtlLookupFunctionEntry(ControlPC, &ImageBase, NULL);
  if (entry == NULL) return 0;
  RtlVirtualUnwind(0, ImageBase, ControlPC, entry, context, &HandlerData, &EstablisherFrame, NULL);
  return (void*)((ULONG64)entry->BeginAddress + ImageBase);
}

void ctx_save(void *ctx) {
  CONTEXT *context = (CONTEXT*)ctx;
  context->ContextFlags = CONTEXT_ALL;
  RtlCaptureContext(context);
  //fprintf(stderr, "%p\n", ctx_ip(ctx));
  ctx_unwind(ctx);
  //fprintf(stderr, "%p\n", ctx_ip(ctx));
}

void ctx_load(void *ctx) {
  CONTEXT *context = (CONTEXT*)ctx;
  RtlRestoreContext(context, NULL);
}

void *ctx_ip(void *ctx) {
  return (void*)((CONTEXT*)ctx)->Rip;
}

void *ctx_sp(void *ctx) {
  return (void*)((CONTEXT*)ctx)->Rsp;
}

static int (*ctx_error_handler)(ctx_error_t *info);

static LONG WINAPI windows_exception_handler(EXCEPTION_POINTERS *ExceptionInfo) {
  ctx_error_t error;
  memset(&error, 0, sizeof(ctx_error_t));
  LONG r = EXCEPTION_EXECUTE_HANDLER; //by default abort execution
  error.ctx = ExceptionInfo->ContextRecord;
  //error.ip = (intptr_t)ExceptionInfo->ExceptionRecord->ExceptionAddress;
  switch(ExceptionInfo->ExceptionRecord->ExceptionCode) {
  //case EXCEPTION_IN_PAGE_ERROR:
  case EXCEPTION_ACCESS_VIOLATION:
    error.id = CTXE_ACCESS;
    error.text = "access violation";
    error.mem = (void*)ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
    break;
  case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    error.id = CTXE_OTHER;
    error.text = "array bounds exceed";
    break;
  case EXCEPTION_BREAKPOINT:
    error.id = CTXE_OTHER;
    error.text = "breakpoint";
    break;
  case EXCEPTION_INT_DIVIDE_BY_ZERO:
    error.id = CTXE_DIV_BY_ZERO;
    error.text = "division by zero";
    break;
  case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    error.id = CTXE_DIV_BY_ZERO_FPU;
    error.text = "division by zero (floating point)";
    break;
  case EXCEPTION_STACK_OVERFLOW:
    error.id = CTXE_STACK_OVERFLOW;
    error.text = "stack overflow";
    break;
  default:
    error.id = CTXE_OTHER;
    error.text = "unknown";
    break;
  }
  if (ctx_error_handler(&error) == CTXE_CONTINUE) {
    r = EXCEPTION_CONTINUE_EXECUTION; // retry execution, hoping that user has fixed error
  } else {
    r = EXCEPTION_EXECUTE_HANDLER; // abort execution
  }
  return r;
}

void ctx_set_error_handler(int (*error_handler)(ctx_error_t *info)) {
  if (!ctx_error_handler) {
    SetUnhandledExceptionFilter(windows_exception_handler);
  }
  ctx_error_handler = error_handler;
}
