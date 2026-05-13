/* arch_arm32.c — AArch32 (ARMv7-A and later).
 *
 * Two variants in play:
 *
 *   APCS-32 softfp — RISC OS native. Floats are passed in the
 *     same r0..r3 + stack pipeline as ints. Doubles consume two
 *     adjacent slots (r0:r1, r2:r3) and require 8-byte alignment.
 *
 *   AAPCS-VFP (hardfp) — modern Linux ARM, Android. Float / double
 *     args use s0..s15 / d0..d7 separately; ints use r0..r3.
 *
 * RISC OS is the immediate target so we'll write softfp first.
 * The variant is selected at compile time via __ARM_PCS_VFP (set
 * by GCC when -mfloat-abi=hard, unset for softfp). The hardfp
 * variant can be folded in here later or split to a sibling file.
 *
 * Calling convention summary (APCS-32 softfp):
 *
 *   - r0..r3 hold the first 4 word-sized args. Doubles occupy two
 *     adjacent regs starting at an even index; if r0..r3 doesn't
 *     contain an aligned pair, the double spills to stack.
 *
 *   - Stack args grow downward, 4-aligned. Doubles 8-aligned.
 *
 *   - Stack must be 8-aligned at the `bl` instruction (AAPCS
 *     requirement; APCS-32 was looser but modern toolchains
 *     enforce it).
 *
 *   - Return: r0 (int / pointer / U32), r0:r1 (long long), r0:r1
 *     (double — softfp packed), r0 (float — softfp).
 *
 * Stub for now; brought up alongside the RISC OS port.
 */

#include "sffi.h"

#if !defined(__arm__) && !defined(_M_ARM)
#  error "arch_arm32.c built but target is not AArch32"
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
