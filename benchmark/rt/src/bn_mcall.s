// bn_mcall -- method call overhead.
//
// Exercises MCALL with a warm method cache: the same method
// is dispatched on the same type over and over so MCACHE_HIT
// fires every time. The remaining cost is dispatch + call
// overhead + the method body. Stresses RT-5 and the threaded
// dispatch's prediction of MCALL -> CALL_RET sequences.

export bn_mcall

N      1000000
WARMUP 10000

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/op"

bn_mcall =
  say "\[bn_mcall\]"
  say "# N=[N] warmup=[WARMUP]"

  // ---- INT.NEG: warm method cache on int.neg ----
  S 0
  times I WARMUP: S = (S + I.neg.neg)

  T0 clock
  S1 0
  times I N: S1 = (S1 + I.neg.neg)
  T1 clock
  report "negneg" (T1 - T0)
  when S1 < 0: say "  (negneg neg unexpected)"

  // ---- INT.ABS: warm cache on int.abs (no args) ----
  S 0
  times I WARMUP: S = (S + I.abs)

  T0 clock
  S2 0
  times I N: S2 = (S2 + I.abs)
  T1 clock
  report "abs   " (T1 - T0)
  when S2 < 0: say "  (abs neg overflow ok)"

  say ""
