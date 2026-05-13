// bn_generic -- AM_GENERIC mode benchmarks.
//
// AM_GENERIC is dh_t (nh_t-template, Robin Hood, 75% load).
// Triggered by any non-int / non-text key, or by a table that
// promoted from one of the simpler modes after a mixed-key
// write. Uses dhEqual_ + dhHash_, which since AM-3 inline the
// int/text fast paths but fall back to MCALL for user types
// like lists and closures.
//
// We use list keys here -- the slowest realistic path. Every
// hash and equality call goes through MCALL (api.m_hash /
// api.m_equal) and method dispatch.

export bn_generic

N      10000
WARMUP 200

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/op"

bn_generic =
  say "\[bn_generic\]"
  say "# N=[N] warmup=[WARMUP]"

  Keys (map I N: [I I])

  T (!)
  // Force GENERIC mode by setting the first key (a list) before
  // warming the loop.
  T.[\sentinel] = \init

  times I WARMUP:
    K Keys[I]
    T.K = I

  T.clear
  T.[\sentinel] = \init

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
  MissKeys (map I N: [(I + N) (I + N)])
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
  T0 clock
  times I N: T.del Keys[I]
  T1 clock
  report "del   " (T1 - T0)

  // ---- ITERATE ----
  T.clear
  T.[\sentinel] = \init
  times I N:
    K Keys[I]
    T.K = (I + 1)

  T0 clock
  L T.l
  S 0
  for [_ V] L:
    when V.is_int: S = (S + V)
  T1 clock
  report "iter  " (T1 - T0)
  when S < 0: say "  (impossible: iter-sum=[S])"

  say ""
