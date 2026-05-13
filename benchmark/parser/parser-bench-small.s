// parser-bench-small.s -- parser throughput for a small script
// (representative of REPL / `-f throwaway.s` interactive use).

Sources [
  'symta/examples/00-hello.s'
  'symta/examples/01-arithmetic.s'
  'symta/examples/04-lists.s'
  'symta/examples/05-loops.s'
  'symta/examples/06-pattern.s'
]

Texts map P Sources: P.get.utf8
Total Texts{&~s+?.n}
say "5 files, total [Total] bytes"

// Warm up.
times W 3: for T Texts: T.parse('bench')

N 100
say "running [N] passes (each passes all 5 files)..."

T0 clock
times I N:
  for T Texts: T.parse('bench')
T1 clock

Elapsed T1 - T0
NCalls N * Texts.n
PerCall Elapsed / NCalls.float
BytesPerSec (Total.float * N.float) / Elapsed

say "elapsed:    [Elapsed] s"
say "per call:   [PerCall * 1000.0] ms"
say "throughput: [(BytesPerSec / 1024.0).int] KB/s"
