// 32-gc-stress.s -- exercise the GC's failure modes.
//
// Doubles as documentation ("here are the things that can break
// if your GC changes go wrong") and as a regression fixture for
// the runtime cache-locality work (RT-4 through RT-8).  Each
// numbered section pins one invariant that an invasive runtime
// restructure could easily violate without the test catching it.
//
// All output is allocation-order-independent: counts and sums,
// never identity-revealing addresses.  Finalizer-firing order
// is stable enough within one run that we can print per-finalizer
// markers, but where order could legitimately drift we accumulate
// instead.
//
// Run:  symta -f examples/32-gc-stress.s


// ----------------------------------------------------------------
// 1. Finalizer firing after explicit gc().  Validates that the
//    sweep visits every unreferenced object and invokes its
//    finalizer exactly once.
//
//    NOTE: the finalizer must be an *inline* `| X => ...` closure.
//    Passing a top-level named function (`fire X = ...;
//    set_finalizer Obj fire`) now rejects with a clean error
//    pointing at the call site (the runtime used to segfault from
//    gc_finalizers' CALL on the symbol; landed as the
//    set_finalizer typecheck guard).
// ----------------------------------------------------------------
type handle id
type point x y

FCount 0

say "[1] finalizer firing under gc():"
times I 32:
  H handle I
  set_finalizer H | X => FCount = FCount + 1
gc
gc                                         // second pass: sweep
                                           // tenured stragglers.
// Anywhere from N-1 to N firings is acceptable -- the most
// recent allocation is typically still pinned by the runtime's
// last-set register.  We assert "almost all" instead of an exact
// count so the test stays stable across allocator variations.
// Use `got` to coerce the comparison result to a 0/1 boolean
// instead of int-returning-self semantics, so the golden line
// stays stable regardless of FCount's exact value.
say "  finalizers fired (>=30): [got(FCount >> 30)]"
say ""


// ----------------------------------------------------------------
// 2. Generation promotion.  Hold a reference through several
//    GC cycles; the object migrates from gen0 -> gen1 -> ... and
//    its content must survive every move.  Validates O_AGE
//    update + GC_REC's already-moved detection + the heap-copy
//    path.
// ----------------------------------------------------------------
say "[2] generation promotion preserves content:"
P point 41 1                               // small heap object;
                                           // referenced by P below
gc
gc
gc
say "  P.x + P.y = [P.x + P.y]"            // must still be 42
say ""


// ----------------------------------------------------------------
// 3. Write barrier (old <- young).  Allocate the container in
//    gen0, force-promote it, then store a fresh young object
//    into it, GC, and verify the young value survives.  The
//    barrier must remember the modified page so the gen0 sweep
//    sees the old container still references the young value.
// ----------------------------------------------------------------
say "[3] old-list keeps young child across gc():"
Old map I 4: 0
gc                                         // promote Old to gen1
gc
times I 4: Old[I] = point I (I*10)         // young points written
                                           // into old slots
gc                                         // sweep young gen;
                                           // pages dirtied by the
                                           // stores must be
                                           // rescanned as roots
Sum 0
times I 4:
  P Old[I]
  Sum = Sum + P.x + P.y
say "  sum of x+y over 4 entries = [Sum]"  // 0+0 + 1+10 + 2+20 + 3+30 = 66
say ""


// ----------------------------------------------------------------
// 4. Deep recursion + GC at the bottom.  Frame walk must visit
//    every active call's locals.  If a frame is missed the
//    locals get overwritten as garbage and the unwinding chain
//    returns nonsense.
// ----------------------------------------------------------------
say "[4] deep-stack frame walk:"

deep_recurse Depth Acc =
  when Depth << 0:                         // Symta: `<<` is "<="
    gc                                     // collect with 32
                                           // pending frames live
    ret Acc
  P point Depth (Depth*2)                  // young allocation per
                                           // frame, becomes part
                                           // of the frame's locals
  Sub deep_recurse (Depth - 1) (Acc + P.x + P.y)
  Sub

R deep_recurse 32 0
// Sum_{d=1..32} (d + 2*d) = 3 * sum_{d=1..32} d = 3 * 528 = 1584
say "  recurse(32) = [R]"
say ""


// ----------------------------------------------------------------
// 5. Many roots.  Build a list of N heap objects, force gc()
//    multiple times, then verify every entry is still readable.
//    Validates the root-set walk over an arglist-style structure.
// ----------------------------------------------------------------
say "[5] many roots preserved across repeated gc():"
Roots map I 200: point I (I+1)
gc
gc
gc
RSum 0
times I 200:
  P Roots[I]
  RSum = RSum + P.x + P.y
// sum_{i=0..199} (i + i+1) = 2*sum(0..199) + 200 = 2*19900 + 200 = 40000
say "  sum across 200 roots = [RSum]"
say ""


// ----------------------------------------------------------------
// 6. Closure captures.  A finalizer that captures the parent's
//    integer state -- the closure must keep the parent alive
//    long enough for the finalizer to run, AND the captured
//    int (immediate, not a heap pointer) must read back
//    unchanged.
// ----------------------------------------------------------------
say "[6] closures pin captured state:"
CCount 0
times I 10:
  Tag I + 1                                // local immediate copy
  H handle Tag
  set_finalizer H | X => CCount = CCount + Tag
gc
gc
// Captured Tags 1..10 sum to 55; at least 9 of 10 should fire
// (last one may stay pinned).  Accept 45..55.
say "  captured-tag sum >= 45: [got(CCount >> 45)]"
say ""


// ----------------------------------------------------------------
// 7. `fin Cleanup Body` (CORE-2) -- the try/finally form.  Cleanup
//    must fire on BOTH the normal-exit and the unwound paths, in
//    that order.  Validates the SBC_CTX SET/REMOVE_UNWIND_HANDLER
//    pair and the btjump finalizer drain.
// ----------------------------------------------------------------
say "[7] fin runs cleanup on normal exit:"
Trace7a 0
RN fin (Trace7a = Trace7a + 1) (Trace7a = 10; 42)
// Order: body sets Trace7a=10, then cleanup increments to 11.
say "  result=[RN], trace=[Trace7a]"

say "[8] fin runs cleanup on btjump (caught error):"
Trace8 0
R8 btrap: => fin (Trace8 = 99) (bad "boom from fin body")
// Cleanup must fire DURING unwind, before btrap returns.
say "  btrap caught: [R8.is_bterror], text=[R8.text], trace=[Trace8]"

say "[9] fin with gen0 pressure -- handler survives a GC pause:"
Trace9 0
// Tighten gen0 so even a small alloc inside the body triggers a
// collection.  The unwind-handler slot lives on api.uwhs / api.puwh
// which the GC root scan walks; a missed slot would mean cleanup
// never fires or fires on garbage.
PrevPages gc_set_gen0_pages 4
press_one =
  Zs map J 8: J
  Zs.n

press_and_bad =
  times I 200: press_one
  bad "after pressure"
R9 btrap: => fin (Trace9 = 77) press_and_bad()
gc_set_gen0_pages PrevPages
say "  caught: [R9.is_bterror], cleanup fired=[Trace9]"

say "done"
