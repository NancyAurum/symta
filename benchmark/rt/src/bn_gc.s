// bn_gc -- GC-pressure and write-barrier benchmarks.
//
// Targets the hot paths RT-4 (O_AGE / write barrier),
// RT-5 (closure CALL dispatch through hooks_heap), RT-6/7
// (megamorphic MCALL miss path), and RT-8 (GC pause cost on a
// deep stack).  These are deliberately not part of bn_loop /
// bn_call / bn_mcall because they exercise the *cache-cold*
// or *cross-line* variants of those paths -- e.g. bn_mcall is
// monomorphic-warm, this one is megamorphic.
//
// Each kernel prints `[bn_gc.<subtest>]` so the run.sh awk
// formatter picks them up the same way as the other groups.

export bn_gc

WBN     500000                              // write-barrier ops
MEGN    500000                              // megamorphic dispatch
GCN     200                                 // gc() calls; deep stack

report Label Elapsed N =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/op"


// ----------------------------------------------------------------
// 1. WRITE BARRIER: store a fresh heap object into a slot of
//    an already-promoted container.  The lsetm barrier should
//    fire on every iteration; that's three cache-line touches
//    on the current layout (RT-4) vs one after the fix.
//
// We promote `Old` past gen0 with a single gc() before the
// timed loop so each iteration's store is the worst case
// (old slot, young value).
// ----------------------------------------------------------------
type cell v
bn_wbarrier =
  say "\[bn_gc.wbarrier\]"
  Old map I 8: cell 0
  gc                                        // promote Old
  // Warm
  times I 200: Old[I % 8] = cell I
  T0 clock
  times I WBN: Old[I % 8] = cell I
  T1 clock
  report "wbarrier" (T1 - T0) WBN


// ----------------------------------------------------------------
// 2. MEGAMORPHIC MCALL: dispatch the same method on alternating
//    types so the inline mcache misses every iteration.  This
//    is the path RT-6 (4 KB methods[] table) and RT-7 (mcache
//    SMC stall on update) both target.
// ----------------------------------------------------------------
type ta x
type tb x
type tc x
type td x
bn_megamcall =
  say "\[bn_gc.megamcall\]"
  // 4 distinct types; rotate so mcache never hits the same one
  // twice in a row.  The mcache slot updates every iteration.
  make_one I =
    M I % 4
    if M >< 0 then ta I
    elif M >< 1 then tb I
    elif M >< 2 then tc I
    else td I
  As map I 16: make_one I
  // Warm: also makes sure each .x is resolved at least once
  Warm 0
  times I 5000: Warm = Warm + As[I % 16].x
  T0 clock
  S 0
  times I MEGN: S = S + As[I % 16].x
  T1 clock
  when S < 0: say "  (sum neg unexpected)"
  report "megamcall" (T1 - T0) MEGN


// ----------------------------------------------------------------
// 3. CLOSURE DISPATCH: chase a chain of distinct closures so
//    hooks_heap entries are all different -- this is the worst
//    case for RT-5 (the per-call hooks_heap indirection).
// ----------------------------------------------------------------
make_adder K = | X => X + K

bn_closure =
  say "\[bn_gc.closure\]"
  // Build 8 closures with different captured constants
  Fs map I 8: make_adder I
  Warm 0
  times I 1000: Warm = Fs[I % 8](Warm)
  T0 clock
  S 0
  times I MEGN: S = Fs[I % 8](S)
  T1 clock
  when S < 0: say "  (chain sum neg unexpected)"
  report "closure " (T1 - T0) MEGN


// ----------------------------------------------------------------
// 4. GC PAUSE on a deep stack.  Build a deep activation chain,
//    fire gc() at the bottom, measure the round-trip cost.
//    Targets RT-8 (frame_t walk during root scan).
// ----------------------------------------------------------------
deep_gc Depth =
  when Depth << 0:
    gc
    ret 0
  // Allocate per-frame so the locals scan has real refs to chase
  C cell Depth
  R deep_gc (Depth - 1)
  R + C.v
bn_deepgc =
  say "\[bn_gc.deepgc\]"
  // Warm to stabilise heap state
  times I 5: deep_gc 64
  T0 clock
  times I GCN: deep_gc 64
  T1 clock
  // ns per gc() call (not per opcode); the 64-frame walk is the
  // unit being measured.  Reported in the same format as the rest
  // of the suite so run.sh's awk picks it up; the column header
  // is misleading ("ns/op") but the magnitude (~µs) reads right.
  report "deepgc  " (T1 - T0) GCN


bn_gc =
  bn_wbarrier
  bn_megamcall
  bn_closure
  bn_deepgc
  say ""
