/* arch_x86_win.c — i386 Windows (cdecl, Win95/98/NT/XP+).
 *
 * Calling convention summary:
 *
 *   - All args pushed right-to-left on stack.
 *   - cdecl: caller cleans up (post-call `add esp, N`). The default
 *     for modern user-compiled DLLs.
 *   - stdcall: callee cleans up via `ret N`. The Windows API
 *     (kernel32, gdi32, user32, etc.) uses stdcall — the function
 *     prototype carries `WINAPI` / `__stdcall`. If you call a
 *     stdcall function with our cdecl-style "caller cleans" stub,
 *     the stack ends up double-popped. Symta's FFI today doesn't
 *     distinguish; SDL and our other deps are all cdecl. If a user
 *     ever wires up a stdcall Win API call we'll need to extend
 *     sffi_type with a calling-convention flag, or add a flavour
 *     per sig.
 *
 *   - Floats and doubles passed at natural size (4 / 8 bytes).
 *   - 64-bit ints pushed as two 32-bit slots, low then high.
 *     Symta doesn't expose 64-bit FFI args today.
 *
 *   - Stack must be 4-aligned at the call site. (No 16-alignment
 *     requirement on i386 Windows.)
 *
 *   - Return: int / pointer in eax; long long in eax:edx; float /
 *     double in st0 (x87 top of stack). We pop st0 into the
 *     return slot ourselves.
 *
 * This is a stub: declared but not yet wired to a working asm
 * implementation. Implementing it is one of the FFI-1 sub-tasks
 * for the Win95 PR target.
 */

#include "sffi.h"

#if !defined(__i386__) && !defined(_M_IX86)
#  error "arch_x86_win.c built but target is not i386"
#endif

int sffi_arch_plan(sffi_sig_t *sig) {
  (void)sig;
  /* TODO: classify each arg by size; doubles get 2 slots, floats 1,
   * everything else 1. Compute total_bytes, store. */
  return -1;
}

uint64_t sffi_arch_call(const sffi_sig_t *sig, void *target,
                        const uint64_t *args) {
  (void)sig; (void)target; (void)args;
  /* TODO: push args right-to-left, call *target, capture eax:edx
   * (or fld st0), restore esp. */
  return 0;
}
