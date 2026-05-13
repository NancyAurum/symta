// bn_call -- function call overhead.
//
// Each iteration calls a small Symta function. Exercises
// CALL / SUBR / LEAVE / ARGLIST. The callee body is tiny so
// the measurement is dominated by call setup + dispatch.

export bn_call

N      2000000
WARMUP 20000

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/op"

add1_ X = X + 1

xorf_ X Y = X -*- Y

bn_call =
  say "\[bn_call\]"
  say "# N=[N] warmup=[WARMUP]"

  // ---- 1ARG: f(x) = x + 1 ----
  S 0
  times I WARMUP: S = add1_ S

  T0 clock
  S1 0
  times I N: S1 = add1_ S1
  T1 clock
  report "1arg " (T1 - T0)
  when S1 <> N: say "  (1arg mismatch: [S1] vs [N])"

  // ---- 2ARG: f(x,y) = x -*- y  (xor avoids overflow noise) ----
  S 0
  times I WARMUP: S = xorf_ S I

  T0 clock
  S2 0
  times I N: S2 = xorf_ S2 I
  T1 clock
  report "2arg " (T1 - T0)
  when S2 < 0: say "  (2arg xor unexpected: [S2])"

  say ""
