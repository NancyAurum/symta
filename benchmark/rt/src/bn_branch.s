// bn_branch -- branchy hot loop.
//
// Exercises FXNLT / FXNGT / B / IFFXN dispatch. The branches
// alternate predictably (50/50 by parity of I), which keeps
// the underlying CPU branch predictor from being the bottleneck
// -- the goal is to stress the *dispatch* indirect, not the
// data-dependent branch.

export bn_branch

N      5000000
WARMUP 50000

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/op"

bn_branch =
  say "\[bn_branch\]"
  say "# N=[N] warmup=[WARMUP]"

  // ---- IFELSE: alternating branch on parity of I ----
  S 0
  times I WARMUP:
    if (I -+- 1) >< 0
      then S = (S + 1)
      else S = (S - 1)

  T0 clock
  S1 0
  times I N:
    if (I -+- 1) >< 0
      then S1 = (S1 + 1)
      else S1 = (S1 - 1)
  T1 clock
  report "alt  " (T1 - T0)

  // ---- TRIBR: three-way branch on I mod 3 ----
  T0 clock
  S2 0
  times I N:
    R (I % 3)
    case R:
      0 = S2 = (S2 + 1)
      1 = S2 = (S2 + 2)
      Else = S2 = (S2 + 3)
  T1 clock
  report "tri3 " (T1 - T0)
  when S2 < 0: say "  (tri3 neg unexpected)"

  say ""
