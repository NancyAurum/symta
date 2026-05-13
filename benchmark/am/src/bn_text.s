// bn_text -- AM_TEXT mode benchmarks.
//
// AM_TEXT is used by literal table syntax (`@t: name!Nancy
// age!37 ...`) and string-keyed lookup tables. Still on stb_ds
// today (AM-6b in TODO.md is blocked on the insertion-order
// contract). The benchmarks here cover insert / lookup hit /
// lookup miss / del / iterate, with keys generated as text so
// shget paths get exercised.
//
// We use a smaller N than bn_int because text key allocation
// (alloc_text on every "key_[I]" string) dominates the
// per-op cost.

export bn_text

N      30000
WARMUP 300

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/op"

bn_text =
  say "\[bn_text\]"
  say "# N=[N] warmup=[WARMUP]"

  // Pre-build the key list so we're benching the AM ops, not
  // the text concatenation. The list itself lives in cache for
  // the duration of the bench.
  Keys (map I N: "key_[I]")

  T (!)
  // Warmup -- also confirms the first WARMUP keys are addable.
  times I WARMUP:
    K Keys[I]
    T.K = I
  T.clear

  // ---- INSERT ----
  T0 clock
  times I N:
    K Keys[I]
    T.K = (I + 1)
  T1 clock
  report "insert" (T1 - T0)

  // ---- LOOKUP HIT ----
  Sum 0
  times I WARMUP: Sum = (Sum + T.(Keys[I]))

  T0 clock
  times I N: Sum = (Sum + T.(Keys[I]))
  T1 clock
  report "hit   " (T1 - T0)
  when Sum < 0: say "  (impossible: sum=[Sum])"

  // ---- LOOKUP MISS ----
  MissKeys (map I N: "miss_[I]")
  Sum2 0
  times I WARMUP:
    V T.(MissKeys[I])
    when V <> No: Sum2 = (Sum2 + 1)
  T0 clock
  times I N:
    V T.(MissKeys[I])
    when V <> No: Sum2 = (Sum2 + 1)
  T1 clock
  report "miss  " (T1 - T0)
  when Sum2 > 0: say "  (impossible: miss-sum=[Sum2])"

  // ---- DEL ----
  Ks T.ks
  T0 clock
  for K Ks: T.del K
  T1 clock
  report "del   " (T1 - T0)
  when T.n <> 0: say "  (post-del n=[T.n], expected 0)"

  // ---- ITERATE ----
  times I N:
    K Keys[I]
    T.K = (I + 1)

  T0 clock
  L T.l
  S 0
  for [_ V] L: S = (S + V)
  T1 clock
  report "iter  " (T1 - T0)
  when S < 0: say "  (impossible: iter-sum=[S])"

  say ""
