/* sffi — Symta's in-house Foreign Function Interface dispatcher.
 *
 * See ARCHITECTURE.md in this directory for the design rationale;
 * this header is the API surface.
 *
 * Three calls. That's it.
 *
 *   sffi_bind(ret_type, n_args, arg_types) -> sffi_sig_t*
 *     Pre-classify a signature. Called once per FFI declaration at
 *     sbc_prepare time. Allocates and returns an opaque-to-callers
 *     sffi_sig_t; the caller stores the pointer and reuses it for
 *     every call to the same FFI signature.
 *
 *   sffi_call(sig, target, args) -> uint64_t
 *     The hot path. Pulls args from a uint64_t[] buffer, loads them
 *     into the right per-ABI argument registers / stack slots, does
 *     the indirect call, returns whatever rax/xmm0/r0 holds. The
 *     caller reinterprets the uint64_t per the return type.
 *
 *   sffi_free(sig)
 *     Releases a sig. Called when an SBC unloads.
 *
 * Licence: dual MIT / Apache-2, same as the rest of Symta. No
 * upstream attribution constraints. Implementation is sffi.c plus
 * exactly one arch_<abi>.c selected at build time.
 */
#ifndef SFFI_H
#define SFFI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Argument and return-value types. Maps 1:1 onto Symta's
 * SBC_NFI_<type> opcodes (see runtime/sif.h). Order kept aligned
 * with the SBC opcodes to make the SBC->sffi conversion trivial. */
typedef enum {
  SFFI_VOID = 0,
  SFFI_I32,            /* int     — sign-extended into the slot */
  SFFI_PTR,            /* void*   — including char* (TXT) */
  SFFI_U32,            /* uint32  — zero-extended */
  SFFI_F32,            /* float   — bit pattern in low 32 of the slot */
  SFFI_F64,            /* double  — bit pattern in full 64 of the slot */
  /* Reserved for future expansion: */
  /* SFFI_I8, SFFI_I16, SFFI_I64, SFFI_U64, SFFI_LDBL */
  SFFI__N
} sffi_type;

/* Maximum number of arguments. Matches NFI_MAX_ARGS in symta.h.
 * Bigger than any FFI Symta declares in practice. */
#define SFFI_MAX_ARGS 32

/* Pre-classified signature. The fields up to `arg_types` are public
 * — they're exactly the input arguments to sffi_bind. The rest is
 * scratch space the backend fills in sffi_arch_plan() and reads in
 * sffi_arch_call(). Callers must treat them as opaque.
 *
 * Allocated by sffi_bind, freed by sffi_free. Internally,
 * sffi_bind calloc()s the struct and dispatches to the backend's
 * sffi_arch_plan() to fill the rest in. */
typedef struct sffi_sig {
  /* === Public (set by sffi_bind from caller args) === */
  uint8_t  ret_type;             /* sffi_type */
  uint8_t  n_args;
  uint8_t  arg_types[SFFI_MAX_ARGS];

  /* === ABI-private (filled by sffi_arch_plan) === */
  /* Total bytes of stack arguments, already rounded up to 16-byte
   * alignment so the call site doesn't need to do it. */
  uint16_t stk_bytes;

  /* Per-arg encoded location. The encoding is private to each
   * backend; sffi.c doesn't interpret it. By convention:
   *   0x00..0x07 = "argument register slot N" (interpretation per ABI)
   *   0x10..0x17 = "float register slot N" (where the ABI separates pools)
   *   0x20+      = "stack offset, in bytes, divided by 8 and offset by 4"
   *
   * Backends that don't have separate float pools (Win64) only use
   * the first range. Backends with no register parameter passing at
   * all (i386 cdecl/stdcall) only use the stack range. */
  uint8_t  arg_loc[SFFI_MAX_ARGS];

  /* Some backends (notably AArch32 softfp) need a "consumed gp regs"
   * counter to handle paired-register doubles. Stash here. */
  uint8_t  gp_consumed;
  uint8_t  fp_consumed;

  /* Backend may stash up to one void* of state. Currently unused on
   * the x86-64 backends; here for forward compatibility. */
  void    *backend_state;
} sffi_sig_t;


/* ============================================================
 * Public API — what runtime/sbc.c calls.
 * ============================================================ */

/* Allocate and pre-classify a signature.
 *
 * `ret`       — return type (SFFI_VOID if no return value)
 * `n_args`    — number of arguments (0..SFFI_MAX_ARGS)
 * `arg_types` — array of `n_args` sffi_type values, or NULL if
 *               n_args == 0
 *
 * Returns a freshly-allocated sig the caller stores. NULL on
 * failure (n_args too large, calloc failed, or the backend rejects
 * the signature — e.g. asking AArch32 softfp for a long double).
 *
 * The returned pointer is stable for the lifetime of the sig; the
 * backend does not move or realloc it. */
sffi_sig_t *sffi_bind(sffi_type ret, int n_args,
                      const sffi_type *arg_types);

/* The hot path. Call `target` with arguments in `args[]`, marshalled
 * per `sig`. Each slot of `args[]` is a `uint64_t` holding the bit
 * pattern of one argument:
 *
 *   - I32  / U32  → value sign- or zero-extended to 64 bits
 *   - PTR         → raw pointer bits
 *   - F32         → IEEE-754 single in the low 32 bits
 *   - F64         → IEEE-754 double in all 64 bits
 *
 * The return is the raw bits the callee left in the ABI's return
 * register (rax / xmm0 / r0 / s0 depending on type). For F32 the
 * bits sit in the low 32; for VOID the value is undefined and the
 * caller must ignore it. */
uint64_t sffi_call(const sffi_sig_t *sig, void *target,
                   const uint64_t *args);

/* Release a sig. Idempotent on NULL. */
void sffi_free(sffi_sig_t *sig);


/* ============================================================
 * Backend contract — what each arch_<abi>.c implements.
 *
 * These are NOT for runtime/sbc.c to call directly. sffi.c (the
 * common dispatcher) calls into them from sffi_bind / sffi_call.
 * ============================================================ */

/* Pre-classify each argument's location for this ABI. Reads
 * sig->ret_type, sig->n_args, sig->arg_types; writes sig->arg_loc,
 * sig->stk_bytes, sig->gp_consumed, sig->fp_consumed,
 * sig->backend_state.
 *
 * Returns 0 on success, -1 on "this ABI cannot represent this
 * signature" (e.g. long double on a 32-bit float-only ABI). */
int sffi_arch_plan(sffi_sig_t *sig);

/* The actual call. Implementations live in arch_<abi>.c. */
uint64_t sffi_arch_call(const sffi_sig_t *sig, void *target,
                        const uint64_t *args);

/* Release any backend-private resources. Optional; default
 * implementation in sffi.c does free(sig->backend_state). Override
 * if a backend allocates more elaborate state. */
void sffi_arch_free(sffi_sig_t *sig);


#ifdef __cplusplus
}
#endif

#endif /* SFFI_H */
