// bn_wh -- weak hashtable throughput on assorted key patterns.
//
// The keys are HEAP OBJECTS (the wh table identity-hashes via the
// raw 64-bit dyn bits; immediates can't be wh keys).  We can't pick
// arbitrary pointer values, but we CAN vary the allocation pattern
// to see whether the gid-sequence the GC hands out interacts badly
// with the ih_t Murmur3-finaliser hash:
//
//   random   -- shuffle order, large gaps between conceptually
//               adjacent inserts (the "control" -- best case).
//   seq      -- alloc N lists, key them in alloc order.  This is
//               the natural pattern the parser hits.
//   churn    -- alloc/free/alloc again, gids get recycled.
//   stride   -- alloc many lists, key only every k'th (gaps).
//
// For each pattern we measure insert, lookup-hit, lookup-miss in
// ns/op, and report the post-insert table fill factor.
//
// Run via benchmark/wh/run.sh.

export bn_wh

N      50000
WARMUP 1000

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "  [Label] [N]: [Elapsed] s, [Per.int] ns/op"


// -----------------------------------------------------------------
// 1. Sequential allocation.  Build N fresh 1-element lists in a
//    tight loop; gid increases by a fixed amount per allocation.
// -----------------------------------------------------------------
bn_seq =
  say "\[seq\]"
  wh_clear_
  Keys (!)
  // Pre-allocate keys outside the timed loop so timer captures only
  // the wh_set_ cost, not the LIST allocation.
  times I N: Keys.I = [I]
  Vals (!)
  times I N: Vals.I = [I 'src']

  T0 clock
  times I N: wh_set_(Keys.I Vals.I)
  T1 clock
  report "insert" (T1 - T0)
  say "  n = [wh_n_()]"

  Sum 0
  times I WARMUP:
    V wh_get_(Keys.I)
    when V <> No: Sum = Sum+1

  T0 clock
  times I N:
    V wh_get_(Keys.I)
    when V <> No: Sum = Sum+1
  T1 clock
  report "hit   " (T1 - T0)

  // Miss: query a key that's never been inserted.
  Misses (!)
  times I N: Misses.I = [I 'miss']
  Sum2 0
  times I WARMUP:
    V wh_get_(Misses.I)
    when V <> No: Sum2 = Sum2+1
  T0 clock
  times I N:
    V wh_get_(Misses.I)
    when V <> No: Sum2 = Sum2+1
  T1 clock
  report "miss  " (T1 - T0)
  when Sum2 > 0: say "  (unexpected miss-hits: [Sum2])"
  say ""


// -----------------------------------------------------------------
// 2. Churn pattern.  Allocate, set, drop ref, GC -- repeated --
//    so the gid recycler is exercised.  This stresses the
//    weak-key sweep path, NOT just steady-state lookup.
// -----------------------------------------------------------------
bn_churn =
  say "\[churn\]"
  wh_clear_

  T0 clock
  Cycles 20
  Batch (N / Cycles)
  times C Cycles:
    LocalKeys (!)
    times I Batch: LocalKeys.I = [I C]
    times I Batch: wh_set_(LocalKeys.I [I 'src'])
    LocalKeys = No                  // drop refs to this batch's keys
    gc()                            // sweep drops the entries
  T1 clock
  Elapsed T1 - T0
  Per (Elapsed * 1000000000.0) / N.float
  say "  [Cycles] cycles x [Batch] entries = [N] inserts + 20 gcs"
  say "  total [Elapsed] s, [Per.int] ns/op (insert+gc-sweep amortised)"
  say "  post-churn n = [wh_n_()]"
  say ""


// -----------------------------------------------------------------
// 3. Held-only stress.  Hold strong refs to every key, fill the
//    table to N entries, do M random lookups.  Tests steady-state
//    cache behaviour vs working-set size.
// -----------------------------------------------------------------
bn_held =
  say "\[held\]"
  wh_clear_
  Keys (!)
  times I N: Keys.I = [I 'held']
  times I N: wh_set_(Keys.I [I 'src'])
  say "  n = [wh_n_()]"

  // Random-ish access pattern -- LCG to scatter the probes across
  // the table so the per-lookup cache miss rate is realistic for
  // an interpreter walking an AST in source order (which IS
  // sequential, but with the GC heap layout the keys-by-gid land
  // in the same cluster of pages).
  Mul 2654435761
  Sum 0
  times I WARMUP:
    K Keys.((I * Mul) -*- (N - 1))
    V wh_get_(K)
    when V <> No: Sum = Sum+1
  T0 clock
  times I N:
    K Keys.((I * Mul) -*- (N - 1))
    V wh_get_(K)
    when V <> No: Sum = Sum+1
  T1 clock
  report "rand-hit" (T1 - T0)

  T0 clock
  Sum2 0
  times I N:
    K Keys.I
    V wh_get_(K)
    when V <> No: Sum2 = Sum2+1
  T1 clock
  report "seq-hit " (T1 - T0)

  say ""


// -----------------------------------------------------------------
// 4. Repeated lookup of the SAME key.  Best-case scenario: the
//    slot stays hot in the cache, every probe is one memory load.
//    If this is slow, the issue is dispatch overhead, not hashing.
// -----------------------------------------------------------------
bn_hot =
  say "\[hot single key\]"
  wh_clear_
  K [42 'hot']
  wh_set_(K [99 'src'])

  T0 clock
  times I N: wh_get_(K)
  T1 clock
  report "same-key" (T1 - T0)
  say ""


bn_wh =
  say "# N=[N] warmup=[WARMUP]"
  bn_seq
  bn_churn
  bn_held
  bn_hot
