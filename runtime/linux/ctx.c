/* runtime/linux/ctx.c — Linux fault-handler glue.
 *
 * Mirrors runtime/osx/ctx.c. The only Linux-specific bit is how we
 * pull the instruction-pointer / stack-pointer out of the ucontext
 * the kernel hands the signal handler: on Linux that's
 * uc_mcontext.gregs[REG_RIP] / gregs[REG_RSP] (vs. Apple's
 * uc_mcontext->__ss.__rip).
 *
 * Everything else — siginfo dispatch, the alternate signal stack,
 * the per-signal description text — is portable POSIX and intentionally
 * kept in lock-step with the osx variant so behaviour matches across
 * unices.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <ucontext.h>

#include "ctx.h"

void *ctx_function_at(void *ip) {
  return 0;
}

void *ctx_module_at(void *ip) {
  return 0;
}

void *ctx_unwind(void *ctx) {
  return 0;
}

void ctx_save(void *ctx) {
}

void ctx_load(void *ctx) {
}

void *ctx_ip(void *ctx) {
  return (void*)((ucontext_t*)ctx)->uc_mcontext.gregs[REG_RIP];
}

void *ctx_sp(void *ctx) {
  return (void*)((ucontext_t*)ctx)->uc_mcontext.gregs[REG_RSP];
}

static int (*ctx_error_handler)(ctx_error_t *info);


static void unix_exception_handler(int sig, siginfo_t *siginfo, void *context) {
  ctx_error_t error;
  memset(&error, 0, sizeof(ctx_error_t));
  error.id = CTXE_OTHER;
  error.mem = 0;
  error.ctx = context;

  switch (sig) {
  case SIGSEGV:
    error.id = CTXE_ACCESS;
    error.text = "access violation";
    error.mem = (void*)(siginfo->si_addr);
    break;
  case SIGBUS:
    error.id = CTXE_ACCESS;
    error.text = "access violation (SIGBUS)";
    error.mem = (void*)(siginfo->si_addr);
    break;
  case SIGINT:
    error.text = "SIGINT (ctrl-c)";
    break;
  case SIGFPE:
    error.mem = (void*)(siginfo->si_addr);
    switch (siginfo->si_code) {
    case FPE_INTDIV:
      error.id = CTXE_DIV_BY_ZERO;
      error.text = "SIGFPE: integer divide by zero";
      break;
    case FPE_INTOVF:
      error.text = "SIGFPE: integer overflow";
      break;
    case FPE_FLTDIV:
      error.id = CTXE_DIV_BY_ZERO_FPU;
      error.text = "SIGFPE: floating-point divide by zero";
      break;
    case FPE_FLTOVF:
      error.text = "SIGFPE: floating-point overflow";
      break;
    case FPE_FLTUND:
      error.text = "SIGFPE: floating-point underflow";
      break;
    case FPE_FLTRES:
      error.text = "SIGFPE: floating-point inexact result";
      break;
    case FPE_FLTINV:
      error.text = "SIGFPE: floating-point invalid operation";
      break;
    case FPE_FLTSUB:
      error.text = "SIGFPE: subscript out of range";
      break;
    default:
      error.text = "SIGFPE: arithmetic exception";
      break;
    }
    break;
  case SIGILL:
    switch (siginfo->si_code) {
    case ILL_ILLOPC: error.text = "SIGILL: illegal opcode"; break;
    case ILL_ILLOPN: error.text = "SIGILL: illegal operand"; break;
    case ILL_ILLADR: error.text = "SIGILL: illegal addressing mode"; break;
    case ILL_ILLTRP: error.text = "SIGILL: illegal trap"; break;
    case ILL_PRVOPC: error.text = "SIGILL: privileged opcode"; break;
    case ILL_PRVREG: error.text = "SIGILL: privileged register"; break;
    case ILL_COPROC: error.text = "SIGILL: coprocessor error"; break;
    case ILL_BADSTK: error.text = "SIGILL: internal stack error"; break;
    default:         error.text = "SIGILL: illegal instruction"; break;
    }
    break;
  case SIGTERM:
    error.text = "SIGTERM: got request to finish execution";
    break;
  case SIGABRT:
    error.text = "SIGABRT: abort() or assert()";
    break;
  default:
    error.text = "unknown";
    break;
  }

  if (ctx_error_handler(&error) == CTXE_CONTINUE) {
    exit(1); /* FIXME: retry-execution path not implemented on Linux */
  } else {
    exit(1);
  }
}

static struct sigaction sa;

/* SIGSTKSZ on glibc 2.34+ is `sysconf(_SC_SIGSTKSZ)` -- a function
 * call, not a compile-time constant -- so we can't use it as an
 * array dimension at file scope. Use a comfortably-large fixed
 * size; if a future signal handler genuinely needs more we can
 * sysconf at runtime. 32 KiB is enough for our error-printer to
 * format a stack trace and exit. */
#define SYMTA_ALTSTACK_SZ (32 * 1024)
static uint8_t alternate_stack[SYMTA_ALTSTACK_SZ];

static void saerr(char *hname) {
  fprintf(stderr, "sigaction failed to set %s handler\n", hname);
  exit(1);
}

void ctx_set_error_handler(int (*error_handler)(ctx_error_t *info)) {
  if (!ctx_error_handler) {
    /* Run handlers on an alternate stack so we survive stack
     * overflows (signal would otherwise grow the bad stack). */
    stack_t ss = {0};
    ss.ss_sp = (void*)alternate_stack;
    ss.ss_size = SYMTA_ALTSTACK_SZ;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, NULL) != 0) {
      fprintf(stderr, "Failed to setup signal handler stack\n");
      exit(1);
    }

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sa.sa_sigaction = unix_exception_handler;

    if (sigaction(SIGSEGV, &sa, NULL)) saerr("SIGSEGV");
    if (sigaction(SIGBUS,  &sa, NULL)) saerr("SIGBUS");
    if (sigaction(SIGFPE,  &sa, NULL)) saerr("SIGFPE");
    if (sigaction(SIGINT,  &sa, NULL)) saerr("SIGINT");
    if (sigaction(SIGILL,  &sa, NULL)) saerr("SIGILL");
  }
  ctx_error_handler = error_handler;
}
