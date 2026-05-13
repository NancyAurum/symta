// bn_arith -- mixed integer arithmetic.
//
// Exercises FXNADD / FXNSUB / FXNMUL / FXNDIV in rolling
// patterns. The body is intentionally branch-free so the
// dispatcher's per-opcode prediction sees the same op
// sequence over and over -- the exact workload that benefits
// from threaded dispatch's per-site indirect branches.

export bn_arith

N      5000000
WARMUP 50000

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/op"

bn_arith =
  say "\[bn_arith\]"
  say "# N=[N] warmup=[WARMUP]"

  // ---- MIX1: addmul -- (S + I) * 3 ----
  S 0
  times I WARMUP: S = ((S + I) * 3)

  T0 clock
  S1 0
  times I N: S1 = ((S1 + I) * 3)
  T1 clock
  report "mix1 " (T1 - T0)
  when S1 < 0: say "  (mix1 overflow ok)"

  // ---- MIX2: addsub -- (S + I) - (I / 2) ----
  S 0
  times I WARMUP: S = ((S + I) - (I / 2))

  T0 clock
  S2 0
  times I N: S2 = ((S2 + I) - (I / 2))
  T1 clock
  report "mix2 " (T1 - T0)
  when S2 < 0: say "  (mix2 neg ok)"

  // ---- BITS: xor + and pattern using Symta's bitwise ops ----
  // Symta bitwise:  -+- AND   -^- OR   -*- XOR   -<- SHL   ->- SHR
  S 0
  times I WARMUP: S = (S -*- I) -+- 0xFFFFFF

  T0 clock
  S3 0
  times I N: S3 = (S3 -*- I) -+- 0xFFFFFF
  T1 clock
  report "bits " (T1 - T0)
  when S3 < 0: say "  (bits neg unexpected)"

  say ""
