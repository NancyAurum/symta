/* arch_x64_sysv.c — x86-64 System V AMD64 calling convention.
 *
 * Linux, macOS, FreeBSD, the BSDs in general.
 * https://gitlab.com/x86-psABIs/x86-64-ABI
 *
 * Summary of the contract:
 *
 *   - Two SEPARATE register pools:
 *       integer pool: rdi, rsi, rdx, rcx, r8, r9    (6 regs)
 *       float pool:   xmm0..xmm7                    (8 regs)
 *     Floats fill the xmm pool independently of where the int pool
 *     sits. This is the key behavioural difference from Win64.
 *
 *   - Beyond the pool capacity, args spill to the stack at
 *     [rsp + 0], 8-byte aligned, in argument order.
 *
 *   - Stack must be 16-aligned at the `call` site. The `call`
 *     instruction pushes 8 bytes for the return address, so the
 *     callee entry sees rsp%16==8.
 *
 *   - NO shadow space — that's a Win64-only thing.
 *
 *   - Return: int / pointer / U32 in rax; F32 / F64 in xmm0.
 *
 *   - Variadic functions: %al holds the number of vector registers
 *     used (for use by va_arg). sffi does not support variadic
 *     FFI, so we always set %al=0.
 *
 * Implementation: same shape as arch_x64_win.c (frame struct
 * accessed through rbx, rbp saves rsp), but the register-fill
 * dispatch is different: separate counters for int and float regs.
 */

#include <stdio.h>
#include "sffi.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if !defined(__x86_64__) && !defined(_M_X64)
#  error "arch_x64_sysv.c built but target is not x86-64"
#endif
#if defined(_WIN64) || defined(__CYGWIN__)
#  error "arch_x64_sysv.c built but target is Windows — wrong backend"
#endif

#define MAX_INT_REGS  6
#define MAX_FLT_REGS  8

/* Argument-location encoding:
 *
 *   0x00..0x05  → int reg slot 0..5    (rdi, rsi, rdx, rcx, r8, r9)
 *   0x10..0x17  → float reg slot 0..7  (xmm0..xmm7)
 *   0x80+(N)    → on stack, frame.stk_args[N]
 */
#define LOC_IREG(slot) ((uint8_t)(slot))
#define LOC_FREG(slot) ((uint8_t)(0x10 | (slot)))
#define LOC_STK(off8)  ((uint8_t)(0x80 | (off8)))


int sffi_arch_plan(sffi_sig_t *sig) {
  int ireg = 0, freg = 0, stk = 0;
  for (int i = 0; i < sig->n_args; i++) {
    uint8_t t = sig->arg_types[i];
    if (t == SFFI_VOID) return -1;
    if (t >= SFFI__N)   return -1;
    int is_float = (t == SFFI_F32 || t == SFFI_F64);
    if (is_float) {
      if (freg < MAX_FLT_REGS) {
        sig->arg_loc[i] = LOC_FREG(freg++);
      } else {
        sig->arg_loc[i] = LOC_STK(stk++);
      }
    } else {
      if (ireg < MAX_INT_REGS) {
        sig->arg_loc[i] = LOC_IREG(ireg++);
      } else {
        sig->arg_loc[i] = LOC_STK(stk++);
      }
    }
  }
  /* Stack: no shadow space. Just the stack args, rounded to 16. */
  int total = stk * 8;
  if (total & 15) total += 8;
  sig->stk_bytes = (uint16_t)total;
  sig->gp_consumed = (uint8_t)ireg;
  sig->fp_consumed = (uint8_t)freg;
  return 0;
}


typedef struct {
  void    *target;
  int64_t  stk_bytes;
  int64_t  n_stk;
  uint64_t i[MAX_INT_REGS];   /* rdi, rsi, rdx, rcx, r8, r9 */
  uint64_t f[MAX_FLT_REGS];   /* xmm0..xmm7 */
  uint64_t stk_args[SFFI_MAX_ARGS];
  uint64_t ret_rax;
  uint64_t ret_xmm0;
} sffi_x64_sysv_frame_t;


uint64_t sffi_arch_call(const sffi_sig_t *sig, void *target,
                        const uint64_t *args) {
  sffi_x64_sysv_frame_t frame;
  memset(&frame, 0, sizeof(frame));
  frame.target    = target;
  frame.stk_bytes = (int64_t)sig->stk_bytes;

  int ireg = 0, freg = 0, stk = 0;
  int n_args = sig->n_args;
  for (int i = 0; i < n_args; i++) {
    uint8_t t = sig->arg_types[i];
    uint64_t v = args[i];
    int is_float = (t == SFFI_F32 || t == SFFI_F64);
    if (is_float && freg < MAX_FLT_REGS) {
      frame.f[freg++] = v;
    } else if (!is_float && ireg < MAX_INT_REGS) {
      frame.i[ireg++] = v;
    } else {
      frame.stk_args[stk++] = v;
    }
  }
  frame.n_stk = stk;

  /* CRITICAL — push rsp well past `frame` BEFORE the asm carves
   * its call-site stack region.
   *
   * Without this, the C compiler positions `frame` (408 B) so that
   * roughly half of it sits below the function's rsp, in the SysV
   * 128-byte red zone. The asm's `subq stk_bytes, %rsp` then lands
   * inside frame.i[3..5]'s storage. The subsequent `rep movsq`
   * writes the stack args we read from frame.stk_args back into
   * frame.i[3..5] — clobbering int-register args 4..6 with stack
   * args 7..9. Reproduces as a +9 offset on c_sum_i32_12 (1..12
   * sums to 78, but we get 87 because args 4,5,6 are replaced by
   * 7,8,9 before the asm reads them into r8/r9/rcx).
   *
   * Bumping rsp down past sizeof(frame) (plus a safety margin)
   * guarantees frame is entirely ABOVE the call-site stack region.
   * volatile + the dummy store keep the compiler from elision. */
  volatile uint64_t pad[SFFI_MAX_ARGS + 8];
  pad[0] = 0;
  (void)pad;

  __asm__ volatile (
      /* See the long comment in arch_x64_win.c::sffi_arch_call for
       * why frame_addr must land in %rbx BEFORE we touch %rbp: the
       * compiler is allowed to allocate %rbp for the [frame_addr]
       * "r" input (rbp is not in our clobber list), so by the time
       * we did `movq frame_addr, %rbx` after `movq %rsp, %rbp`, rbp
       * would already be overwritten and rbx would point at random
       * stack memory. Same risk on every SysV ABI we target. */
      "pushq %%rbx\n\t"
      "movq  %[frame_addr], %%rbx\n\t"    /* rbx = &frame (safe — rbp untouched) */
      "pushq %%rbp\n\t"
      "movq  %%rsp, %%rbp\n\t"

      /* Align rsp to 16 and carve stk_bytes. */
      "andq  $-16, %%rsp\n\t"
      "subq  %c[ofs_stk_bytes](%%rbx), %%rsp\n\t"

      /* Copy stack args to [rsp .. ]. SysV has no shadow space, so
       * args start right at rsp. */
      "movq  %c[ofs_n_stk](%%rbx), %%rcx\n\t"
      "testq %%rcx, %%rcx\n\t"
      "jz    1f\n\t"
      "leaq  %c[ofs_stk_args](%%rbx), %%rsi\n\t"
      "movq  %%rsp, %%rdi\n\t"
      "cld\n\t"
      "rep   movsq\n\t"
      "1:\n\t"

      /* Integer pool: rdi, rsi, rdx, rcx, r8, r9. */
      "movq  %c[ofs_i0](%%rbx), %%rdi\n\t"
      "movq  %c[ofs_i1](%%rbx), %%rsi\n\t"
      "movq  %c[ofs_i2](%%rbx), %%rdx\n\t"
      "movq  %c[ofs_i3](%%rbx), %%rcx\n\t"
      "movq  %c[ofs_i4](%%rbx), %%r8\n\t"
      "movq  %c[ofs_i5](%%rbx), %%r9\n\t"

      /* Float pool: xmm0..xmm7. */
      "movq  %c[ofs_f0](%%rbx), %%xmm0\n\t"
      "movq  %c[ofs_f1](%%rbx), %%xmm1\n\t"
      "movq  %c[ofs_f2](%%rbx), %%xmm2\n\t"
      "movq  %c[ofs_f3](%%rbx), %%xmm3\n\t"
      "movq  %c[ofs_f4](%%rbx), %%xmm4\n\t"
      "movq  %c[ofs_f5](%%rbx), %%xmm5\n\t"
      "movq  %c[ofs_f6](%%rbx), %%xmm6\n\t"
      "movq  %c[ofs_f7](%%rbx), %%xmm7\n\t"

      /* Variadic-call AL convention: we don't do variadic, set 0. */
      "xorl  %%eax, %%eax\n\t"

      "callq *%c[ofs_target](%%rbx)\n\t"

      "movq  %%rax,  %c[ofs_ret_rax] (%%rbx)\n\t"
      "movq  %%xmm0, %c[ofs_ret_xmm0](%%rbx)\n\t"

      "movq  %%rbp, %%rsp\n\t"
      "popq  %%rbp\n\t"
      "popq  %%rbx\n\t"

      :
      : [frame_addr]     "r"  (&frame),
        [ofs_target]     "i"  (offsetof(sffi_x64_sysv_frame_t, target)),
        [ofs_stk_bytes]  "i"  (offsetof(sffi_x64_sysv_frame_t, stk_bytes)),
        [ofs_n_stk]      "i"  (offsetof(sffi_x64_sysv_frame_t, n_stk)),
        [ofs_i0]         "i"  (offsetof(sffi_x64_sysv_frame_t, i[0])),
        [ofs_i1]         "i"  (offsetof(sffi_x64_sysv_frame_t, i[1])),
        [ofs_i2]         "i"  (offsetof(sffi_x64_sysv_frame_t, i[2])),
        [ofs_i3]         "i"  (offsetof(sffi_x64_sysv_frame_t, i[3])),
        [ofs_i4]         "i"  (offsetof(sffi_x64_sysv_frame_t, i[4])),
        [ofs_i5]         "i"  (offsetof(sffi_x64_sysv_frame_t, i[5])),
        [ofs_f0]         "i"  (offsetof(sffi_x64_sysv_frame_t, f[0])),
        [ofs_f1]         "i"  (offsetof(sffi_x64_sysv_frame_t, f[1])),
        [ofs_f2]         "i"  (offsetof(sffi_x64_sysv_frame_t, f[2])),
        [ofs_f3]         "i"  (offsetof(sffi_x64_sysv_frame_t, f[3])),
        [ofs_f4]         "i"  (offsetof(sffi_x64_sysv_frame_t, f[4])),
        [ofs_f5]         "i"  (offsetof(sffi_x64_sysv_frame_t, f[5])),
        [ofs_f6]         "i"  (offsetof(sffi_x64_sysv_frame_t, f[6])),
        [ofs_f7]         "i"  (offsetof(sffi_x64_sysv_frame_t, f[7])),
        [ofs_stk_args]   "i"  (offsetof(sffi_x64_sysv_frame_t, stk_args)),
        [ofs_ret_rax]    "i"  (offsetof(sffi_x64_sysv_frame_t, ret_rax)),
        [ofs_ret_xmm0]   "i"  (offsetof(sffi_x64_sysv_frame_t, ret_xmm0))
      : "memory", "cc",
        "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
  );

  if (sig->ret_type == SFFI_F32 || sig->ret_type == SFFI_F64) {
    return frame.ret_xmm0;
  }
  return frame.ret_rax;
}
