// 24-runtime.s -- talking to the runtime: GC, finalizers, reflection
//
// Symta's runtime exposes a small surface area for inspecting and
// hooking into the GC. The headline use case is releasing C-side
// resources (file handles, FFI pointers, mutexes) when the Symta
// object that owned them becomes unreachable -- without `with`
// blocks, ref-counting, or "remember to call .free" discipline.
//
// Run:  symta -f examples/24-runtime.s


// ----------------------------------------------------------------
// 1. Heap vs. immediate. Most small values (int, float, fixtext,
//    No, type tags) live in the 64-bit dyn word itself -- no heap
//    allocation, no GC pressure. `imm_ X` returns 1 for those.
// ----------------------------------------------------------------
say "immediates (no heap):"
say "  imm_ 5    = [imm_ 5]"
say "  imm_ 1.0  = [imm_ 1.0]"
say "  imm_ 'a'  = [imm_ 'a']                  // short fixtext"
say "  imm_ No   = [imm_ No]"
LongTxt 'longer text on heap'
LL [1 2 3]
say "  imm_ '[LongTxt]' = [imm_ LongTxt]"
say "  imm_ [LL]    = [imm_ LL]"
say ""


// ----------------------------------------------------------------
// 2. Runtime types. `typename X` returns a short symbol; method
//    dispatch goes through this. `methods_ X` enumerates every
//    method visible on X's type.
// ----------------------------------------------------------------
say "typename 5     = [typename 5]"
say "typename 1.0   = [typename 1.0]"
say "typename 'hi'  = [typename 'hi']"
LL [1 2 3]
say "typename a list = [typename LL]"

type point x y
P point 1 2
say "typename P     = [typename P]"
M methods_ P
say "P has [M.n] methods total"
First5 M[:5]{?0}
say "first 5 names: [First5]"
say ""


// ----------------------------------------------------------------
// 3. Finalizers. `set_finalizer Obj Fn` registers Fn to be invoked
//    once when the GC notices Obj has become unreachable. Fn
//    receives the about-to-be-freed object as its single argument.
//
//    The crucial property: this is *deterministic enough* for
//    bookkeeping (release a file handle, decrement a refcount,
//    log a removal) but NOT prompt -- the call happens whenever
//    the next collection sweeps the right generation.
// ----------------------------------------------------------------

// A toy "managed handle". The C side here is replaced by a plain
// integer; in real FFI code this would be a malloc'd pointer.
type handle id

HandleCount 0
make_handle =
  HandleCount = HandleCount + 1
  Id HandleCount
  H handle Id                              // type handle id
  // Capture Id in the closure, since the object's fields may
  // already be partially reclaimed by the time the finalizer runs.
  set_finalizer H | X => say "  GC: closing handle [Id]"
  H


// Allocate a few short-lived handles. Each one becomes unreachable
// at the end of the loop iteration; the next GC collects them.
say "allocating 3 handles, dropping refs immediately..."
times I 3:
  H make_handle
  say "  using handle [H.id]"
  // H goes out of scope at the loop iteration end

// Trigger a GC cycle by allocating enough garbage to fill gen0.
// (A dedicated `gc()` builtin isn't exposed; allocation pressure
// is the portable way to nudge the collector.)
say "triggering GC..."
times I 10000: dup K 100 K
say "done"
say ""


// ----------------------------------------------------------------
// 4. `rtstat` -- runtime self-report. Shows heap usage, generation
//    sizes, type table size, etc. Useful when you're tuning a hot
//    loop or wondering why memory is climbing.
// ----------------------------------------------------------------
say "rtstat output:"
rtstat
