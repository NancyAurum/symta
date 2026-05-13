// bn_list -- list construction + index ops.
//
// Allocates a fixed-size list once, then hammers index get/set
// in tight loops. Exercises LIST / FXNLGET / FXNLSET / FXNSIZE
// in the dispatch loop.

export bn_list

N      2000000
WARMUP 20000

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/op"

bn_list =
  say "\[bn_list\]"
  say "# N=[N] warmup=[WARMUP]"

  // ---- IDXGET: list[i mod 16] in a loop ----
  Xs (map I 16: I)

  S 0
  times I WARMUP: S = (S + Xs[I % 16])

  T0 clock
  S1 0
  times I N: S1 = (S1 + Xs[I % 16])
  T1 clock
  report "iget " (T1 - T0)
  when S1 < 0: say "  (iget neg unexpected)"

  // ---- IDXSET: write to list[i mod 16] ----
  Ys (map I 16: 0)

  times I WARMUP: Ys[I % 16] = I

  T0 clock
  times I N: Ys[I % 16] = I
  T1 clock
  report "iset " (T1 - T0)
  when Ys.n <> 16: say "  (iset size drift: [Ys.n])"

  // ---- LSIZE: .n in a tight loop (size of constant list) ----
  T0 clock
  S2 0
  times I N: S2 = (S2 + Xs.n)
  T1 clock
  report "lsize" (T1 - T0)
  when S2 <> (16 * N): say "  (lsize sum=[S2] expected [16 * N])"

  say ""
