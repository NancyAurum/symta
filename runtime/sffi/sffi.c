/* sffi.c — common dispatcher.
 *
 * Glues the public API (sffi_bind / sffi_call / sffi_free) to the
 * per-ABI backend. Picking the right backend is a build-time
 * decision driven by predefined macros; see the dispatch table at
 * the bottom of this file.
 *
 * The hot path is sffi_call, and it boils down to one indirect call
 * into the backend. We deliberately keep the wrapper minimal so the
 * "common code" has no per-call overhead beyond what the compiler
 * can inline away.
 */

#include "sffi.h"
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Build-time backend selection.
 *
 * Exactly one backend is compiled in. The Makefile picks the right
 * .c file per platform; this dispatch table is here so #include of
 * the wrong file fails loudly rather than silently linking nothing.
 *
 * Order of preference (most specific first):
 *   __x86_64__ + _WIN64           → x64 Windows
 *   __x86_64__                    → x64 SysV (Linux/macOS/BSD)
 *   __i386__   + _WIN32           → x86 Windows (Win95+, cdecl)
 *   __i386__                      → x86 SysV (Linux 32-bit)
 *   __aarch64__                   → AArch64
 *   __arm__                       → AArch32
 *
 * Anything else is a build error — we'd rather know at link time
 * that we forgot a port than silently produce a binary that
 * segfaults on the first FFI call.
 * ============================================================ */

#if defined(__x86_64__) || defined(_M_X64)
#  if defined(_WIN64) || defined(__CYGWIN__)
#    define SFFI_BACKEND "x64-win"
#  else
#    define SFFI_BACKEND "x64-sysv"
#  endif
#elif defined(__i386__) || defined(_M_IX86)
#  if defined(_WIN32)
#    define SFFI_BACKEND "x86-win"
#  else
#    define SFFI_BACKEND "x86-sysv"
#  endif
#elif defined(__aarch64__) || defined(_M_ARM64)
#  define SFFI_BACKEND "arm64"
#elif defined(__arm__) || defined(_M_ARM)
#  define SFFI_BACKEND "arm32"
#else
#  error "sffi: no backend for this target — see runtime/sffi/ARCHITECTURE.md"
#endif


/* ============================================================
 * Public API.
 * ============================================================ */

sffi_sig_t *sffi_bind(sffi_type ret, int n_args,
                      const sffi_type *arg_types) {
  if (n_args < 0 || n_args > SFFI_MAX_ARGS) {
    return NULL;
  }
  if (ret < 0 || ret >= SFFI__N) {
    return NULL;
  }
  for (int i = 0; i < n_args; i++) {
    if (arg_types[i] < 0 || arg_types[i] >= SFFI__N
        || arg_types[i] == SFFI_VOID) {
      return NULL;
    }
  }

  /* calloc so all the ABI-private fields start at zero — the backend
   * fills only what it needs. */
  sffi_sig_t *sig = (sffi_sig_t *)calloc(1, sizeof(sffi_sig_t));
  if (!sig) return NULL;

  sig->ret_type = (uint8_t)ret;
  sig->n_args   = (uint8_t)n_args;
  for (int i = 0; i < n_args; i++) {
    sig->arg_types[i] = (uint8_t)arg_types[i];
  }

  if (sffi_arch_plan(sig) != 0) {
    free(sig);
    return NULL;
  }
  return sig;
}

uint64_t sffi_call(const sffi_sig_t *sig, void *target,
                   const uint64_t *args) {
  /* The hot path. One indirect call into the backend. The backend
   * is responsible for everything: register loads, stack pushes,
   * the indirect call to `target`, return-value extraction. */
  return sffi_arch_call(sig, target, args);
}

void sffi_free(sffi_sig_t *sig) {
  if (!sig) return;
  sffi_arch_free(sig);
  free(sig);
}


/* ============================================================
 * Default sffi_arch_free — backends that don't override.
 * ============================================================ */

#ifndef SFFI_BACKEND_OVERRIDES_FREE
void sffi_arch_free(sffi_sig_t *sig) {
  if (sig && sig->backend_state) {
    free(sig->backend_state);
    sig->backend_state = NULL;
  }
}
#endif


/* ============================================================
 * Diagnostic — useful for "what was sffi compiled for?" probes.
 * ============================================================ */

const char *sffi_backend_name(void) {
  return SFFI_BACKEND;
}
