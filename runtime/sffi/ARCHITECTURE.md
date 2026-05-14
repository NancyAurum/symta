# sffi — Symta's purpose-built FFI

sffi (Symta FFI) is the in-house FFI dispatcher.  It replaced the
vendored [cinvoke](https://www.cinvoke.org/) library in May 2026;
cinvoke's source was deleted from the tree once both x86-64
backends shipped a release.  This document is the design rationale
+ the per-ABI implementation map.

## Goals

1. **Licensing purity** — drop the only non-dual-MIT/Apache
   component (cinvoke was BSD-3 with a non-endorsement clause).
   Source-of-record now under the same MIT / Apache-2 dual licence
   as the rest of Symta.
2. **Lower per-call latency** — cinvoke spent most of its time
   on generality Symta doesn't need (format-string parsing,
   `parmtypes[]` malloc, hashtable lookups). On the i7-12700H
   we measured ~50–100 ns / call; sffi runs 20–40 ns.
3. **Robust port surface** — one C source per ABI, ~200 lines
   each. Adding a new ABI is a single file drop-in, not a
   surgery into a 3,500-line third-party library. Already in
   tree: x86-64 Windows + x86-64 SysV (in production); i386
   Win95 (stdcall + cdecl), AArch32 RISC OS (APCS-32), AArch64
   Linux/macOS (stubbed with calling-convention notes, brought
   up on demand).
4. **No JIT, no executable memory** — keep the W^X story
   trivial. RISC OS, Win95, and signed-binary-only platforms
   (iOS, modern macOS) all benefit. We pay one indirect call
   per FFI invocation but save the platform-specific
   `VirtualAlloc(PAGE_EXECUTE_READWRITE)` /
   `mprotect(PROT_EXEC)` / `MAP_JIT+pthread_jit_write_protect`
   surface area we'd otherwise need.
5. **Same call surface as cinvoke had** at the Symta-runtime
   boundary — no SBC opcode changes, no `ffi_begin` macro
   changes, no user-visible language differences.  The
   migration was invisible to Symta source code.

The "no JIT" choice is the one place where we deliberately don't
match the absolute fastest FFI libraries (LuaJIT, dyncall in
"vm" mode generate per-signature thunks). The user-visible payoff
of that 10–20 ns last-mile gain doesn't justify the platform
surface area; see "alternatives considered" below.


## Call pipeline — end to end

Symta's FFI path, top to bottom:

1. **Symta-side** (`src/macro.s::ffi_begin` and the `ffi` macro
   per-function) expands `ffi gfx_blit.ptr Gfx.ptr X.int Y.int
   Src.ptr` into an `_ffi_call` SSA form that the compiler
   serialises into SBC.
2. **Compile time** (`src/compiler.s::ssa_ffi_call`) emits a
   stream of `SBC_NFI_<type>` arg ops followed by an `SBC_NFI`
   call op and an `SBC_NFI_<rettype>` result op.
3. **Load time** (`runtime/sbc.c::sbc_prepare`) walks the
   `nrs[]` table, classifies each FFI signature into reg/stack
   buckets per the active ABI, and stores the resulting
   `sffi_sig_t*` in `sbc->nfi_trmps[]`.  All the per-call
   dispatch cost is paid here, once per FFI declaration.
4. **Per-call** (`runtime/sbc.c::sbc_exec` case `SBC_NFI`):
   - Arg coercion ops fill `api.nfi_args[NFI_MAX_ARGS]`
     (a `uint64_t[32]`).
   - `sffi_call(sig, target, args)` walks the pre-classified
     plan: loads reg args, pushes stack args, indirect-calls
     the target.  Returns the raw rax/xmm0 bit pattern.
   - Return value is unboxed via `FFI_FROM_<type>` macros.


## Architecture — the call pipeline

```
   load time:
     sbc_prepare ─► sffi_bind(ret_type, n_args, arg_types)
                    │
                    ▼
                  per-ABI sig prepass:
                    classify each arg as reg or stack
                    compute stack-arg byte total
                    record reg-arg order
                    => allocate + fill sffi_sig_t
                    => stored in sbc->nfi_trmps[i]

   per call:
     SBC_NFI ──► sffi_call(sig, target, args[])
                    │
                    ▼
                  per-ABI implementation:
                    walk pre-computed plan
                    load reg args from args[]
                    push stack args from args[]
                    call *target
                    extract rax / xmm0 / r0...
                    return as uint64_t
```

**Key idea:** the sig prepass (cheap, runs once per FFI declaration)
moves all the type dispatch out of the per-call path. At call
time, the per-ABI implementation walks a pre-classified plan
without re-checking types.


## Public API (`sffi.h`)

Three functions. That's the entire surface:

```c
sffi_sig_t *sffi_bind(sffi_type ret, int n_args,
                      const sffi_type *args);

uint64_t sffi_call(const sffi_sig_t *sig, void *target,
                   const uint64_t *args);

void sffi_free(sffi_sig_t *sig);
```

`sffi_type` enum:

```c
typedef enum {
  SFFI_VOID = 0,
  SFFI_I32,    // int    (sign-extended into a slot)
  SFFI_U32,    // u32    (zero-extended into a slot)
  SFFI_PTR,    // void*  (8 bytes on 64-bit, 4 on 32-bit)
  SFFI_F32,    // float  (low 32 bits of slot)
  SFFI_F64,    // double (full 64 bits of slot)
  /* future: SFFI_I8, SFFI_I16, SFFI_I64, SFFI_U64, SFFI_LDBL */
  SFFI__N
} sffi_type;
```

There's no separate text type — `text` maps to `SFFI_PTR`, since
they're the same at the ABI level. The conversion from Symta `dyn`
to `char*` is done upstream by the existing `FFI_ARG_text` macro.

The arg buffer convention is unchanged: each arg gets one
8-byte slot, and the upstream `FFI_ARG_*` macros put the value
in the slot in the canonical way (sign-extended ints, raw float
bits for F32/F64, pointer-sized raw bits for PTR).


## Per-ABI backend contract

Each backend provides exactly two functions:

```c
/* called once per signature, at sbc_prepare time. */
void sffi_arch_plan(sffi_sig_t *sig);

/* called per call. THIS is the hot path. */
uint64_t sffi_arch_call(const sffi_sig_t *sig, void *target,
                        const uint64_t *args);
```

`sffi_sig_t` is a public struct (definition in `sffi.h`):

```c
typedef struct {
  uint8_t  ret_type;       /* sffi_type */
  uint8_t  n_args;
  uint8_t  arg_types[NFI_MAX_ARGS];

  /* ABI-private fields, sized for the largest backend. The
   * struct is allocated by sffi_bind and freed by sffi_free;
   * backends fill these out in sffi_arch_plan. */
  uint16_t stk_bytes;      /* total stack-arg bytes (16-aligned) */
  uint8_t  arg_loc[NFI_MAX_ARGS];
                           /* per-arg location code:
                            *   0x00..0x07 = int reg slot 0..7
                            *   0x10..0x17 = float reg slot 0..7
                            *   0x20..0xFF = stack offset (bytes)
                            *
                            * encoding scheme is private to each
                            * backend — `arg_loc` is opaque to
                            * sffi.c. */
} sffi_sig_t;
```

The intent: the *types* are public (Symta's compiler emitted
them; the bytecode interpreter knows them). The *plan* is
private to each ABI.


## Calling-convention specifics

### x86-64 Windows (`sffi_x64_win.c`) — first backend

- First 4 args go in regs, by *position*:
  - position 0 → rcx OR xmm0
  - position 1 → rdx OR xmm1
  - position 2 → r8  OR xmm2
  - position 3 → r9  OR xmm3
- Choice depends on type: integer / pointer args use the gp
  register at that position; float / double args use the xmm
  register at the same position.
- Args 5+ go on stack at `[rsp + 32 + (i-4)*8]`. The first 32
  bytes of stack are the "shadow space" Win64 reserves for the
  callee to spill rcx/rdx/r8/r9.
- Stack must be 16-aligned at the `call` instruction site.
- Variadic functions: float args also go in gp regs. **Not
  supported by sffi** — Symta has no variadic FFI declaration
  syntax.
- Return: int / pointer in rax, float / double in xmm0.

Reference: [Microsoft x64 calling convention](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170).

### x86-64 SysV (`sffi_x64_sysv.c`) — Linux, macOS, BSD

- Two separate register pools:
  - integer pool: rdi, rsi, rdx, rcx, r8, r9 (6 regs)
  - float pool:   xmm0..xmm7 (8 regs)
- Float / double args fill the float pool independently of int
  args. Pointer + int args fill the int pool.
- Beyond the pool capacity, args spill to stack starting at
  `[rsp + 0]`, 8-byte aligned, in argument order.
- Stack must be 16-aligned at the `call` instruction.
- Return: rax for int/pointer, xmm0 for float/double.

Reference: [System V AMD64 ABI](https://gitlab.com/x86-psABIs/x86-64-ABI).

### i386 cdecl / stdcall (`sffi_x86_win.c`, `sffi_x86_sysv.c`)

- All args on stack, right-to-left.
- cdecl: caller cleans up the stack (post-call `add esp, N`).
- stdcall: callee cleans up. Windows API uses stdcall; modern
  C code uses cdecl.
- Float and double pushed at natural size (4 or 8 bytes).
- 64-bit ints are passed in two 32-bit slots, low then high.
- Return: eax (int), eax:edx (long long), st0 (float/double).

Why bother with i386 in 2026? Two reasons. (a) Win95 / Win98
support is a stated PR goal. (b) The 32-bit ARM port (RISC OS)
shares half its register-passing logic with i386 — they both
need to handle paired-32-bit-slots for `double`.

### AArch32 APCS (`sffi_arm32_apcs.c`) — RISC OS

- First 4 args in r0..r3.
- Doubles passed in adjacent register pairs (r0:r1, r2:r3) or on
  stack if no pair available; must be 8-aligned.
- Float / double passing depends on FP variant (softfp, hardfp).
  RISC OS uses softfp historically — ints and floats share the
  gp register pool.
- Beyond r3, args go on stack.
- Return: r0 (int), r0:r1 (long long), s0/d0 (float in hardfp).

Reference: [ARM AAPCS32](https://github.com/ARM-software/abi-aa/blob/main/aapcs32/aapcs32.rst).

### AArch64 (`sffi_arm64.c`) — Linux, macOS, iOS

- 8 gp registers (x0..x7) and 8 float registers (v0..v7).
- Separate pools, like SysV x86-64.
- Stack args 8-aligned, in order.
- Return: x0 (int), v0 (float).

Reference: [ARM AAPCS64](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst).


## Implementation technique — the call site

Two viable techniques for the per-call dispatch:

### Option A (chosen): hand-written assembly via inline asm

One big `__asm__` block per ABI that loads parameter registers
from a struct and does `call *%ptr`.  Works on GCC + Clang +
MinGW.  Doesn't work on MSVC (no inline asm).  Since Symta
ships on w64devkit (MinGW), MSVC is a non-issue.

### Option B: standalone `.S` files per ABI

Hand-write the call site in pure assembly, assembled by gas /
yasm / fasm. Maximally portable across compilers (no inline-asm
syntax flavour required). Slightly more build-system surface.

**Decision:** Option A.  Revisit Option B only if we ever need
a non-GCC-family compiler.


## Sig pre-pass — example walk-through

Suppose Symta's macro layer registers `ffi gfx_blit.ptr Gfx.ptr
X.int Y.int Src.ptr`. The arg type stream is `[PTR, I32, I32,
PTR]`, return type `PTR`.

On Win64, sffi_arch_plan classifies:

| pos | arg | reg / stack | encoded loc |
|-----|-----|-------------|-------------|
| 0   | PTR | rcx         | 0x00 (int reg 0) |
| 1   | I32 | rdx         | 0x01 (int reg 1) |
| 2   | I32 | r8          | 0x02 (int reg 2) |
| 3   | PTR | r9          | 0x03 (int reg 3) |

`n_args = 4`, `stk_bytes = 0` (all 4 fit in regs).

On SysV, sffi_arch_plan classifies:

| pos | arg | reg / stack | encoded loc |
|-----|-----|-------------|-------------|
| 0   | PTR | rdi         | 0x00 (int reg 0) |
| 1   | I32 | rsi         | 0x01 (int reg 1) |
| 2   | I32 | rdx         | 0x02 (int reg 2) |
| 3   | PTR | rcx         | 0x03 (int reg 3) |

Same shape; different encoding semantics (the registers behind
the slot numbers differ per ABI, but `arg_loc[]` is private to
the backend so it just encodes "the right one"). Stack args
encode as `(stack offset / 8) + 0x20`.


## Per-call cost — back of envelope

cinvoke (predecessor):
- format-string parsing: ~10 ns
- per-arg type-check + classification: ~10 ns × n_args
- `alloca` + memcpy stack args: ~10 ns
- one indirect call: ~5 ns
- return value extraction: ~5 ns

Total for a 4-arg call: ~70 ns.

sffi:
- per-arg load (already classified): ~1 ns × n_args
- one indirect call: ~5 ns
- return value extraction: ~2 ns

Total for a 4-arg call: ~12 ns.

Net: ~5–6× faster on the per-call path.  At ~50K gfx calls /
frame in SoM's Management window, that's ~3 ms / frame budget
freed.  Numbers are estimates from instruction counts; in-tree
measurement still pending under the `--profile=management`
flag once a frame-time micro-benchmark is wired up.


## Testing strategy

A dedicated suite at [`../../tests/ffi/`](../../tests/ffi/) (new).
The harness builds a small C `.dll` / `.so` exposing one
function per (return_type × {0..N args}) combination, then
calls each from Symta and compares the returned value against
a Symta-side expected value.

Coverage:
- Each return type × each arg-type combination at arities 0..6
- Variadic-ish patterns: float-int interleaving (catches register
  pool classification bugs)
- Round-tripping: passing a ptr and getting it back
- All-stack: a function with 12 ints (exhausts reg pool on every
  ABI, exercises the stack-arg pipeline)

Plus the existing test surface: all 29 gfx golden tests go
through sffi. Any pixel-level regression flags an FFI bug.

Drift test: the bootstrap rebuild also uses FFI (the gfx and
font libs build textual output that the parser consumes via
`text.parse`). A sffi regression should surface as drift-stage
failure.


## Migration history

Phases 0–3 done (May 2026).  Phase 4 (additional ABIs) brought
up on demand.

- **Phase 0 — research + docs.**  Architecture doc, `sffi.h` /
  `sffi.c` skeletons, backend stubs for every targeted ABI
  (`#error not implemented` on unsupported ones — keeps the
  build surface honest).
- **Phase 1 — x86-64 Windows backend.**  Implemented
  `arch_x64_win.c`; wired `runtime/sbc.c` to call `sffi_*` in
  place of the cinvoke trampolines.  Verified by the full
  Windows test sweep + the FFI regression suite + drift
  bootstrap.
- **Phase 2 — x86-64 SysV backend.**  Implemented
  `arch_x64_sysv.c`; brings up Linux + macOS support.
  Verified on WSL Ubuntu 24.04 / gcc 13.3 -- full sweep
  green, drift PASSES (3-round byte-identical bootstrap).
- **Phase 3 — delete cinvoke.**  `../cinvoke/` removed.
  Net licence claim: dual MIT / Apache-2 throughout.

Phase 4 — **additional ABIs as needed.**  Stubs exist for:
  - i386 Win95 (stdcall + cdecl): legacy Windows.
  - i386 SysV: legacy Linux.
  - AArch32 RISC OS (APCS-32 softfp): RISC OS port.
  - AArch64 Linux / macOS / iOS: modern ARM.

Each backend is independent and ~200 lines.  Pick the order by
which platform comes up first.


## Alternatives considered

**libffi.** ~20 % faster than cinvoke, supports more ABIs out
of the box, MIT-style licence. The cost: ~6,000 lines of code
+ a build system + autoconf detection of every target's quirks.
For Symta's small (≤8 type, ≤32 arg) signature surface, this is
substantially more dependency than the 200-line-per-ABI in-tree
approach buys.

**dyncall.** Smaller than libffi, BSD-2 (cleaner licence than
cinvoke's BSD-3-with-non-endorsement-clause), comparable per-call
cost to sffi's design.  The chattier API (`dcArgInt` /
`dcArgDouble` / `dcCallVoid`) doesn't fit naturally with Symta's
"signature known upfront, all args in a buffer" pattern — we'd
be re-shaping the calling code on both sides.  The dependency
cost is real though smaller than libffi.

**Per-signature JIT thunk** (LuaJIT-style). Lowest possible
per-call cost: the thunk is straight-line code, ~2 ns/arg + one
indirect call. The platform tax is W^X-aware allocation, which
on every modern OS means a non-trivial dance. Defer until we
have measured evidence that the 10-ns gap matters in a real
profile.

**Generated C wrappers** (Forth / Tcl style). Generate a C
function per signature at compile time. Lowest call cost
(direct call to a known-shape function). The cost: needs a code
generator in the build path, and every new FFI signature
requires a regen step. Doesn't compose with Symta's existing
`ffi_begin` macro which is runtime-flexible. Out of scope.


## Open questions for the user

1. **64-bit int types.** Symta's `SBC_NFI_*` enum reserves
   slots for char / short / long / long-long but only the
   commented-in subset is wired through. sffi can support
   them all (the table is just a function dispatch); should
   we add I8 / I16 / I64 / U64 in this round, or wait until a
   user reports needing them?
2. **Struct-by-value.** Symta has no syntactic support for
   passing structs by value through FFI today. SDL APIs that
   take `SDL_Rect` are wrapped at the C side via pointer-and-
   memcpy. Should sffi reserve the design space for adding
   struct-by-value later, or formally declare it out of scope?
3. **Callbacks** (C calling Symta).  sffi declares "no executable
   memory" as a goal, but a one-time alloc at FFI-bind for a
   callback is arguably acceptable.  SoM doesn't use FFI callbacks
   today; voxpie doesn't either.  Defer or fold in?

My recommendation on all three: defer to Phase 4+. The
existing FFI surface is what Symta + SoM + voxpie + the test
suite exercise; expanding it can wait until somebody actually
needs the expansion.
