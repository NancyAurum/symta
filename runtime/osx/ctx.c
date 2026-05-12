#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "ctx.h"

enum X86Registers {
  GS, FS, ES, DS, EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX,
  TRAPNO, ERR, EIP, CS, EFL, UESP, SS
};

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
  return (void*)((ucontext_t*)ctx)->uc_mcontext->__ss.__rip;
}

void *ctx_sp(void *ctx) {
  return (void*)((ucontext_t*)ctx)->uc_mcontext->__ss.__rsp;
}

static int (*ctx_error_handler)(ctx_error_t *info);


static void unix_exception_handler(int sig, siginfo_t *siginfo, void *context) {
  ctx_error_t error;
  memset(&error, 0, sizeof(ctx_error_t));
  error.id = CTXE_OTHER;
  error.mem = 0;
  error.ctx = context;

  switch(sig) {
  case SIGSEGV: //segmentation fault
    error.id = CTXE_ACCESS;
    error.text = "access violation";
    error.mem = (void*)(siginfo->si_addr);
    break;
  case SIGBUS: //protect memory accessed
    error.id = CTXE_ACCESS;
    error.text = "access violation (SIGBUS)";
    error.mem = (void*)(siginfo->si_addr);
    break;
  case SIGINT:
    error.text = "SIGINT (ctrl-c)";
   break;
  case SIGFPE: //floating point exception
    error.mem = (void*)(siginfo->si_addr);
		switch(siginfo->si_code) {
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
	case SIGILL:
		switch(siginfo->si_code) {
		case ILL_ILLOPC:
      error.text = "SIGILL: illegal opcode";
			break;
		case ILL_ILLOPN:
      error.text = "SIGILL: illegal operand";
			break;
		case ILL_ILLADR:
      error.text = "SIGILL: illegal addressing mode";
			break;
		case ILL_ILLTRP:
      error.text = "SIGILL: illegal trap";
			break;
		case ILL_PRVOPC:
      error.text = "SIGILL: privileged opcode";
			break;
		case ILL_PRVREG:
      error.text = "SIGILL: privileged register";
			break;
		case ILL_COPROC:
      error.text = "SIGILL: coprocessor error";
			break;
		case ILL_BADSTK:
      error.text = "SIGILL: internal stack error";
			break;
		default:
      error.text = "SIGILL: illegal Instruction";
			break;
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
     // retry execution, hoping that user has fixed error
    exit(1); //FIXME
		/*ucontext_t* uc = (ucontext_t*) context;
		void **gregs = uc->uc_mcontext.gregs;
		void *eip = (void*) gregs[X86Registers.EIP],
		void **esp = (void**) gregs[X86Registers.ESP];

		// imitate the effects of "call seghandle_userspace"
		esp --; // decrement stackpointer.
						// remember: stack grows down!
		*esp = eip;
		// set up OS for call via return, like in the attack
		eip = (void*) &seghandle_userspace;*/
  } else {
    exit(1);
  }
}

struct sigaction sa;
static uint8_t alternate_stack[SIGSTKSZ];

static void saerr(char *hname) {
  fprintf(stderr, "sigaction failed to set %s handler\n", hname);
  exit(1);
}

void ctx_set_error_handler(int (*error_handler)(ctx_error_t *info)) {
  if (!ctx_error_handler) {
    /* setup alternate stack */
    {
      stack_t ss = {};
      ss.ss_sp = (void*)alternate_stack;
      ss.ss_size = SIGSTKSZ;
      ss.ss_flags = 0;

      if (sigaltstack(&ss, NULL) != 0) {
        fprintf(stderr, "Failed to setup signal handler stack\n");
        exit(1);
      }
    }

    sigemptyset(&sa.sa_mask);
#ifdef __APPLE__
    /* for some reason we backtrace() doesn't work on osx
       when we use an alternate stack */
    sa.sa_flags = SA_SIGINFO;
#else
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
#endif

    sa.sa_sigaction = unix_exception_handler;
    if (sigaction(SIGSEGV, &sa, NULL)) saerr("SIGSEGV");
    if (sigaction(SIGBUS,  &sa, NULL)) saerr("SIGBUS");
    if (sigaction(SIGFPE,  &sa, NULL)) saerr("SIGFPE");
    if (sigaction(SIGINT,  &sa, NULL)) saerr("SIGINT");
    if (sigaction(SIGILL,  &sa, NULL)) saerr("SIGILL");

    //if (sigaction(SIGTERM, &sa, NULL)) saerr("SIGTERM");
    //if (sigaction(SIGABRT, &sa, NULL)) saerr("SIGABRT");
  }
  ctx_error_handler = error_handler;
}
