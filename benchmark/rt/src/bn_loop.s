// bn_loop -- raw dispatch overhead.
//
// The tightest possible loop: increment a counter N times.
// Dominated by the bytecode dispatcher (B16 + FXNADD +
// ST_x sequence per iteration). The single most sensitive
// benchmark to RT-1's switch-vs-threaded dispatch rewrite,
// because there's almost nothing else for the CPU to do
// between branches.

export bn_loop

N      30000000
WARMUP 100000

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/op"

bn_loop =
  say "\[bn_loop\]"
  say "# N=[N] warmup=[WARMUP]"

  // ---- COUNT: tight increment loop ----
  S 0
  times I WARMUP: S = (S + 1)

  T0 clock
  Sum 0
  times I N: Sum = (Sum + 1)
  T1 clock
  report "count" (T1 - T0)
  when Sum <> N: say "  (impossible: sum=[Sum])"

  // ---- XOR: accumulate by xor with induction variable ----
  // (xor instead of + avoids int overflow at large N, while
  // still preventing the optimizer from DCE'ing the body.)
  S2 0
  times I WARMUP: S2 = (S2 -*- I)

  T0 clock
  S3 0
  times I N: S3 = (S3 -*- I)
  T1 clock
  report "xor  " (T1 - T0)
  when S3 < 0: say "  (xor unexpected: [S3])"

  say ""
