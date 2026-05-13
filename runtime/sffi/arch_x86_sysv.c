/* arch_x86_sysv.c — i386 System V (Linux 32-bit, FreeBSD i386, ...)
 *
 * Same shape as arch_x86_win.c (all-stack args, eax:edx return,
 * x87 float return) but with subtly different rules:
 *
 *   - Stack must be 16-aligned at the `call` site (SysV i386 ABI
 *     v1.0 + draft updates — most distros enforce this since GCC
 *     4.5). Push args, pad to alignment, call, restore.
 *
 *   - All Linux distros use cdecl (caller cleans). No stdcall.
 *
 *   - Floats / doubles same as Windows: pushed at natural size,
 *     returned in st0.
 *
 * Stub for now; brought up alongside arch_x64_sysv.c.
 */

#include "sffi.h"

#if !defined(__i386__) && !defined(_M_IX86)
#  error "arch_x86_sysv.c built but target is not i386"
#endif
#if defined(_WIN32)
#  error "arch_x86_sysv.c built but target is Windows — wrong backend"
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
