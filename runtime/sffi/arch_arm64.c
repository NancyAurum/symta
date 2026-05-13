/* arch_arm64.c — AArch64 (ARMv8-A and later).
 *
 * Linux, macOS (Apple silicon), iOS, Android.
 *
 * Calling convention summary (AAPCS64):
 *
 *   - 8 gp registers for int args (x0..x7).
 *   - 8 SIMD/FP registers for float args (v0..v7 / d0..d7 / s0..s7).
 *   - Separate pools, like SysV x86-64.
 *   - Stack args 8-aligned, in order, starting at sp + 0. Some
 *     platforms (Apple) round each arg to its natural alignment
 *     and then to 8; standard AAPCS64 always 8-aligns.
 *   - Stack must be 16-aligned at the `bl` instruction.
 *   - Return: x0 (int / pointer / U32), x1:x0 if Q-form (128 bit)
 *     return; v0 (float / double).
 *
 * Note: Apple's AAPCS64 variant has a quirk for variadic functions
 * (the last fixed arg is always promoted to the next slot boundary
 * and stack-aligned). sffi has no variadic support so this doesn't
 * apply.
 *
 * Stub for now; needed for any future Linux-ARM64 / macOS-arm64
 * port.
 */

#include "sffi.h"

#if !defined(__aarch64__) && !defined(_M_ARM64)
#  error "arch_arm64.c built but target is not AArch64"
#endif

int sffi_arch_plan(sffi_sig_t *sig) {
  (void)sig;
  return -1;
}

uint64_t sffi_arch_call(const sffi_sig_t *sig, void *target,
                        const uint64_t *args) {
  (void)sig; (void)target; (void)args;
  return 0;
}
