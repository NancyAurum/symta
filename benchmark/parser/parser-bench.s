// parser-bench.s -- compare Symta vs C parser throughput.
//
// Loads a substantial Symta source (reader.s itself, ~14 KB) and
// times text.parse over it repeatedly. The reader.s parse runs the
// whole pipeline: tokenize.c -> add_bars_c_ -> parse_tokens(_c_) ->
// parse_strip -- so the comparison is end-to-end, with the only
// variable being which parse_tokens implementation reader.sbc was
// built against.
//
// Run:
//   make runtime                                 # build symta.exe first
//   ./symta/symta.exe -f benchmark/parser/parser-bench.s

Src 'symta/src/reader.s'.get.utf8
say "source: [Src.n] bytes"

// Warm up so first-call costs don't bias the timing.
times I 3: Src.parse('symta/src/reader.s')

N 200
say "running [N] iterations..."

T0 clock
times I N: Src.parse('symta/src/reader.s')
T1 clock

Elapsed T1 - T0
PerCall Elapsed / N.float
BytesPerSec (Src.n.float * N.float) / Elapsed

say "elapsed:   [Elapsed] s"
say "per call:  [PerCall * 1000.0] ms"
say "throughput: [(BytesPerSec / 1024.0).int] KB/s"
