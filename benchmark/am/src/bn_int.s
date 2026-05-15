// bn_int -- AM_INT mode benchmarks.
//
// AM_INT is the most common adaptive-map mode in real Symta code:
// ECS column tables, gid-keyed indices, anything with int keys
// that isn't 0-or-1. AM-6 replaced its stb_ds backing
// with ih_t (Robin Hood at 75% load). This bench measures the
// resulting per-op cost on the four read/write paths and on
// iteration.

export bn_int

N      100000
WARMUP 1000

// Report one measurement line. ns_per_op is 1e9 * Elapsed / N.
report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/op"

bn_int =
  say "\[bn_int\]"
  say "# N=[N] warmup=[WARMUP]"

  // ---- INSERT: T.I = I+1 (unique int keys, forces grows) ----
  T (!)
  times I WARMUP: T.I = (I + 1)
  T.clear

  T0 clock
  times I N: T.I = (I + 1)
  T1 clock
  report "insert" (T1 - T0)

  // ---- LOOKUP HIT ----
  Sum 0
  times I WARMUP: Sum = (Sum + T.I)

  T0 clock
  times I N: Sum = (Sum + T.I)
  T1 clock
  report "hit   " (T1 - T0)
  when Sum < 0: say "  (impossible: sum=[Sum])"

  // ---- LOOKUP MISS ----
  Sum2 0
  times I WARMUP:
    K (I + N)
    V T.K
    when V <> No: Sum2 = (Sum2 + 1)

  T0 clock
  times I N:
    K (I + N)
    V T.K
    when V <> No: Sum2 = (Sum2 + 1)
  T1 clock
  report "miss  " (T1 - T0)
  when Sum2 > 0: say "  (impossible: miss-sum=[Sum2])"

  // ---- DEL ----
  Keys T.ks
  T0 clock
  for K Keys: T.del K
  T1 clock
  report "del   " (T1 - T0)

  when T.n <> 0: say "  (post-del n=[T.n], expected 0)"

  // ---- ITERATE ----
  times I N: T.I = (I + 1)

  T0 clock
  L T.l
  S 0
  for [_ V] L: S = (S + V)
  T1 clock
  report "iter  " (T1 - T0)
  when S < 0: say "  (impossible: iter-sum=[S])"

  say ""
