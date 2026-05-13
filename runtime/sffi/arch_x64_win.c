/* arch_x64_win.c — x86-64 Microsoft Windows calling convention.
 *
 * https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention
 *
 * Summary of the contract:
 *
 *   - First 4 args go in registers by POSITION:
 *       position 0 → rcx OR xmm0
 *       position 1 → rdx OR xmm1
 *       position 2 → r8  OR xmm2
 *       position 3 → r9  OR xmm3
 *     Choice depends on type: int/pointer/U32/I32 use the gp
 *     register; F32/F64 use the xmm register at the same position.
 *     A position consumes its slot regardless of which family is
 *     used — i.e. (PTR, F64, PTR) consumes rcx, xmm1, r8, not
 *     rcx, xmm0, rdx.
 *
 *   - Args 5+ go on the stack at [rsp + 32 + (i-4)*8]. The 32 bytes
 *     of "shadow space" are reserved by the convention even when
 *     the callee never spills the register args.
 *
 *   - Stack must be 16-aligned at the `call` site. The `call`
 *     instruction pushes 8 bytes for the return address, so the
 *     callee entry sees rsp%16==8.
 *
 *   - Return: int / pointer / U32 in rax; F32 / F64 in xmm0.
 *
 *   - Variadic functions: float args also go to gp regs. NOT
 *     supported by sffi — Symta has no variadic FFI declaration.
 *
 * Implementation notes:
 *
 *   - Per-call state lives in a stack-local struct (`frame`). We
 *     pass its address into the asm block in a register; the asm
 *     accesses all state through that register (which we move into
 *     rbx, a callee-saved we own for the duration). This is the key
 *     trick that lets us re-align rsp without invalidating C-side
 *     stack offsets: nothing in the asm body uses rsp-relative
 *     addressing for state — only `(rbx)`-relative.
 *
 *   - rbx + rbp are pushed/popped around the call so the surrounding
 *     C function's stack-frame addressing survives.
 *
 *   - rsp is saved/restored via "save rsp -> rbp; modify rsp; restore
 *     rsp from rbp at exit". The compiler can't see across the asm;
 *     after we restore, the C frame is intact.
 */

#include "sffi.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if !defined(__x86_64__) && !defined(_M_X64)
#  error "arch_x64_win.c built but target is not x86-64"
#endif
#if !defined(_WIN64) && !defined(__CYGWIN__)
#  error "arch_x64_win.c built but target is not Windows x64"
#endif

/* Argument-location encoding (set by sffi_arch_plan, consumed by
 * sffi_arch_call). Win64 only has two cases:
 *
 *   0x00..0x03  → register slot 0..3 (rcx/rdx/r8/r9 OR xmm0..3,
 *                 chosen by arg_types[i])
 *   0x80+(N)    → on stack, at frame.stk_args[N]
 *
 * Sig planning packs stack args contiguously starting at N=0; the
 * call site copies them out to [rsp + 32 .. ] in order. */
#define LOC_REG(slot)    ((uint8_t)(slot))            /* 0..3 */
#define LOC_STK(off8)    ((uint8_t)(0x80 | (off8)))   /* off8 = qword offset */
#define LOC_IS_REG(loc)  ((loc) < 4)
#define LOC_IS_STK(loc)  (((loc) & 0x80) != 0)
#define LOC_STK_QWORDS(loc) ((int)((loc) & 0x7F))


/* ============================================================
 * sffi_arch_plan — runs once per signature at bind time.
 * ============================================================ */

int sffi_arch_plan(sffi_sig_t *sig) {
  int stk_slots = 0;
  for (int i = 0; i < sig->n_args; i++) {
    uint8_t t = sig->arg_types[i];
    if (t == SFFI_VOID) return -1;
    if (t >= SFFI__N)   return -1;
    if (i < 4) {
      sig->arg_loc[i] = LOC_REG(i);
    } else {
      sig->arg_loc[i] = LOC_STK(stk_slots);
      stk_slots++;
    }
  }
  int total = 32 + stk_slots * 8;
  if (total % 16) total += 8;
  sig->stk_bytes = (uint16_t)total;
  sig->gp_consumed = (uint8_t)(sig->n_args < 4 ? sig->n_args : 4);
  sig->fp_consumed = sig->gp_consumed;  /* same slots, position-based */
  return 0;
}


/* ============================================================
 * sffi_arch_call — the hot path.
 *
 * Layout of `frame` (the per-call asm state struct):
 *
 *   offset  field         purpose
 *   ──────  ───────────   ───────────────────────────────────
 *     0     target        function pointer
 *     8     stk_bytes     bytes to sub from rsp (16-aligned)
 *    16     n_stk         number of stack-arg qwords (0..28)
 *    24     i[0..3]       register arg slots (8B each → 32B)
 *    56     f[0..3]       xmm arg slots      (8B each → 32B)
 *    88     stk_args[28]  stack args, in order, qword each
 *    ...    ret_rax       rax saved after the call
 *    ...    ret_xmm0      xmm0 saved after the call
 *
 * All offsets are computed via offsetof so the struct can be
 * rearranged without touching the asm.
 * ============================================================ */

typedef struct {
  void    *target;
  int64_t  stk_bytes;
  int64_t  n_stk;
  uint64_t i[4];
  uint64_t f[4];
  uint64_t stk_args[SFFI_MAX_ARGS - 4 + 4]; /* +4 for safety */
  uint64_t ret_rax;
  uint64_t ret_xmm0;
} sffi_x64_win_frame_t;

uint64_t sffi_arch_call(const sffi_sig_t *sig, void *target,
                        const uint64_t *args) {
  sffi_x64_win_frame_t frame;
  memset(&frame, 0, sizeof(frame));
  frame.target    = target;
  frame.stk_bytes = (int64_t)sig->stk_bytes;
  frame.n_stk     = (int64_t)((sig->n_args > 4) ? (sig->n_args - 4) : 0);

  int n_args = sig->n_args;
  for (int i = 0; i < n_args; i++) {
    uint8_t t = sig->arg_types[i];
    uint64_t v = args[i];
    if (i < 4) {
      if (t == SFFI_F32 || t == SFFI_F64) {
        frame.f[i] = v;
      } else {
        frame.i[i] = v;
      }
    } else {
      frame.stk_args[i - 4] = v;
    }
  }

  /* The asm uses %c[ofs_X] to emit literal offsets. The %c modifier
   * formats a constant input as a plain number (no `$` prefix), so
   * we can write things like `8(%%rbx)` via `%c[ofs_target](%%rbx)`. */
  __asm__ volatile (
      /* Preserve callee-save we'll trample. rbp gets used to save
       * our entry rsp; rbx is the "base pointer to frame". */
      "pushq %%rbx\n\t"
      "pushq %%rbp\n\t"
      "movq  %%rsp, %%rbp\n\t"            /* rbp = caller's rsp */

      /* Move frame address into rbx (compiler picked some random
       * input register for us). */
      "movq  %[frame_addr], %%rbx\n\t"

      /* Re-align rsp down to 16. Now any subsequent push aligns. */
      "andq  $-16, %%rsp\n\t"

      /* Carve out stk_bytes. The value is pre-aligned to 16. */
      "subq  %c[ofs_stk_bytes](%%rbx), %%rsp\n\t"

      /* Copy stack args from rbx[stk_args] to [rsp + 32 .. ].
       * `rep movsq` consumes rcx qwords from rsi to rdi. */
      "movq  %c[ofs_n_stk](%%rbx), %%rcx\n\t"
      "testq %%rcx, %%rcx\n\t"
      "jz    1f\n\t"
      "leaq  %c[ofs_stk_args](%%rbx), %%rsi\n\t"
      "leaq  32(%%rsp), %%rdi\n\t"
      "cld\n\t"
      "rep   movsq\n\t"
      "1:\n\t"

      /* Load int registers. */
      "movq  %c[ofs_i0](%%rbx), %%rcx\n\t"
      "movq  %c[ofs_i1](%%rbx), %%rdx\n\t"
      "movq  %c[ofs_i2](%%rbx), %%r8\n\t"
      "movq  %c[ofs_i3](%%rbx), %%r9\n\t"

      /* Load xmm registers (one quadword each). */
      "movq  %c[ofs_f0](%%rbx), %%xmm0\n\t"
      "movq  %c[ofs_f1](%%rbx), %%xmm1\n\t"
      "movq  %c[ofs_f2](%%rbx), %%xmm2\n\t"
      "movq  %c[ofs_f3](%%rbx), %%xmm3\n\t"

      /* THE indirect call. rsp is 16-aligned at this instruction. */
      "callq *%c[ofs_target](%%rbx)\n\t"

      /* Save return values. */
      "movq  %%rax,  %c[ofs_ret_rax] (%%rbx)\n\t"
      "movq  %%xmm0, %c[ofs_ret_xmm0](%%rbx)\n\t"

      /* Restore rsp + callee-saves. */
      "movq  %%rbp, %%rsp\n\t"
      "popq  %%rbp\n\t"
      "popq  %%rbx\n\t"

      :
      : [frame_addr]     "r"  (&frame),
        [ofs_target]     "i"  (offsetof(sffi_x64_win_frame_t, target)),
        [ofs_stk_bytes]  "i"  (offsetof(sffi_x64_win_frame_t, stk_bytes)),
        [ofs_n_stk]      "i"  (offsetof(sffi_x64_win_frame_t, n_stk)),
        [ofs_i0]         "i"  (offsetof(sffi_x64_win_frame_t, i[0])),
        [ofs_i1]         "i"  (offsetof(sffi_x64_win_frame_t, i[1])),
        [ofs_i2]         "i"  (offsetof(sffi_x64_win_frame_t, i[2])),
        [ofs_i3]         "i"  (offsetof(sffi_x64_win_frame_t, i[3])),
        [ofs_f0]         "i"  (offsetof(sffi_x64_win_frame_t, f[0])),
        [ofs_f1]         "i"  (offsetof(sffi_x64_win_frame_t, f[1])),
        [ofs_f2]         "i"  (offsetof(sffi_x64_win_frame_t, f[2])),
        [ofs_f3]         "i"  (offsetof(sffi_x64_win_frame_t, f[3])),
        [ofs_stk_args]   "i"  (offsetof(sffi_x64_win_frame_t, stk_args)),
        [ofs_ret_rax]    "i"  (offsetof(sffi_x64_win_frame_t, ret_rax)),
        [ofs_ret_xmm0]   "i"  (offsetof(sffi_x64_win_frame_t, ret_xmm0))
      : "memory", "cc",
        "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11",
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5"
  );

  if (sig->ret_type == SFFI_F32 || sig->ret_type == SFFI_F64) {
    return frame.ret_xmm0;
  }
  return frame.ret_rax;
}
