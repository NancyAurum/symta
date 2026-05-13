# tests/ffi — FFI regression suite

Comprehensive coverage for Symta's FFI dispatcher
([`runtime/sffi/`](../../runtime/sffi/)) across every type and
arity combination the language supports. Designed to catch the
calling-convention bugs that hit when porting to a new ABI
(Linux x86-64, Win95 x86, RISC OS AArch32, AArch64 macOS, ...).

The suite is intentionally redundant in the type-matrix dimension
because the same FFI bug — a register pool exhausted earlier than
expected, a stack-arg slot's high 32 bits left dirty, a return-
value bit-pattern misinterpreted — manifests differently depending
on exactly which (return × arg-pattern × arity) tuple you call
with. Better to fail loudly on one case than mysteriously on
another.


## What it tests

Three tiers. Each is a `tc_<name>` Symta function in
[`src/tc_*.s`](src/), called via the `--case=<name>` flag.

### Tier 1: synthetic library (`cffilib/`)

A small C library, source under [`cffilib/src/cffilib.c`](cffilib/src/cffilib.c),
built locally per platform into `cffilib/lib/cffi.ffi` and staged
into the test project's `ffi/` folder. Every function in cffilib
is deterministic and exists to exercise one specific FFI shape:

| Case | What it pins down |
|------|-------------------|
| `sanity` | The simplest call works (no-arg, returns text). If this fails the dynamic loader is broken. |
| `int_roundtrip` | Identity on i32; constant returns; boundary values (INT_MIN, INT_MAX, -1, 1<<30). |
| `uint32` | u32 identity; the zero-extension into the 64-bit slot is correct; UINT_MAX comes back as 0xFFFFFFFF. |
| `float` | Single-precision round-trip; common values (0.0, 1.0, -2.5, 1e6, π). |
| `double` | Double round-trip; high-magnitude (1e15) and high-precision (full π) values. |
| `ptr` | Pointer encode/decode round-trip; pointer arithmetic via C. |
| `string` | text → const char* marshalling; strlen, strcmp, strchr offsets. |
| `arith` | Two-arg arithmetic in every type combo. Catches "second arg landed in the wrong slot". |
| `arity_i32` | 4, 6, 8, 12 int args. Exercises stack spill on both Win64 and SysV. Includes negative numbers for sign-extension on stack slots. |
| `arity_f64` | 4, 6, 8, 10 double args. Past SysV's xmm0..7 pool. |
| `interleave` | int/float interleaving — the SysV-vs-Win64 distinction. (iffi), (fiif), (ffii), (iiff), and 6-arg patterns. Catches almost every wrong-pool / wrong-slot bug. |
| `buffer` | Two-way buffer round-trip: Symta-alloc + C-fill, then C-alloc + Symta-write. Plus u32 read/write at offset. |
| `str_ops` | strlen / strchr / bytesum across fixtext and bigtext sizes; UTF-8 bytes. |
| `float_edge` | Bit patterns that catch sloppy float marshalling: 1e-300, 1e300, 0.1, the (0.5*2+1)/3 sequence. |
| `stress` | 1000 sequential calls + interleaved int/float calls. Catches per-call state leakage. |

### Tier 2: real-library calls

Test against libraries that exist on every target platform. If
the FFI works for these, it works for any user code.

| Case | What it pins down |
|------|-------------------|
| `libc` | Calls strlen / memset / memcpy from libc / msvcrt / libSystem (whichever the platform supplies). If Symta can't call libc, it can't call anything. |
| `zlib` | Calls zlib functions including the in/out-param `uLong *dstLen` pattern (compress / uncompress). Real-world API shape. |


## Running

```sh
make test-ffi                  # from symta/, runs everything
bash tests/ffi/run.sh          # same, without invoking make
bash tests/ffi/run.sh sanity   # one case
bash tests/ffi/run.sh --update # rewrite goldens after intentional change
```

`run.sh` does the build pipeline:

1. Detect platform; pick `cffilib/Makefile.{w64,osx,(empty)}`.
2. Build `cffilib/lib/cffi.ffi` (the test library).
3. Stage `cffi.ffi` into the test project's `ffi/`.
4. Run `symta .` to compile the test harness.
5. For each case, invoke `go.exe --case=<name>` and diff stdout
   against the golden in `expected/<name>.out`.

A test passes iff actual output matches the golden AND contains
no `FAIL` line. Any case whose golden doesn't yet exist is flagged
as "new" — `--update` captures the current output as the new
golden.


## Adding a new test case

1. Pick a name. Convention: `tc_<purpose>.s` in `src/`.
2. If you need a new C function, add it to `cffilib/src/cffilib.c`
   and declare it in `src/cffi.s` (the Symta-side FFI bindings).
3. Write `src/tc_<name>.s`. It should `export tc_<name>` and
   print one `PASS …` or `FAIL …` line per assertion.
4. Append `tc_<name>` to the `use` line and the `case` dispatch
   in `src/go.s`.
5. Append the basename (without `tc_` prefix) to the `CASES`
   array in `run.sh`.
6. Run `bash tests/ffi/run.sh --update` to capture the golden.

The contract for assertions:

- `say "PASS <label>"` on success.
- `say "FAIL <label>: <detail>"` on failure.
- Never print anything else from `tc_<name>` unless it goes through
  PASS/FAIL labelling.


## What's missing (and why)

- **No `i8` / `i16` / `i64` / `u8` / `u16` / `u64`.** Symta's
  SBC_NFI opcode set commits to int32 / uint32 / float / double /
  ptr / void. Smaller and wider types aren't currently exposed to
  Symta-level FFI declarations. When they get wired in,
  `cffilib.c` is the right place to add round-trip tests.
- **No structs by value.** Out of scope for sffi entirely — see
  the rationale in `runtime/sffi/ARCHITECTURE.md`. SDL_Rect-style
  use cases get wrapped C-side.
- **No callbacks (C calling Symta).** Also out of scope for sffi.
  When it lands, a `tc_callbacks.s` should exercise the
  Symta→C→Symta round-trip via a C function that takes a
  function pointer and invokes it.
- **No variadic functions.** Symta has no `ffi …(…)` syntax for
  declaring variadic foreign functions. Variadic on Win64 has a
  separate ABI quirk (float args also go to gp regs) that we
  don't need to implement until somebody binds `printf`.


## Why this matters for the Linux port

The user's stated goal: bring sffi to ABI completeness so the
Linux x86-64 (SysV) port becomes a matter of running the suite
on the new platform and fixing whatever fails. The cases here
are deliberately calibrated to surface ABI bugs early:

- `interleave` will fail immediately on any backend that
  confuses Win64 slot-position semantics with SysV pool
  semantics.
- `arity_f64` with 10 args will fail on any backend that
  miscounts the xmm pool capacity.
- `arity_i32` with 12 negative ints will fail on any backend
  that doesn't sign-extend stack-slot args properly.
- `stress` will catch any state-leakage between consecutive
  FFI calls.

If all 17 cases pass on a new platform, the FFI dispatcher is
production-quality on that platform. The game and voxpie can
follow without further FFI surgery.
