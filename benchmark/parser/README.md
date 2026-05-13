# Parser Throughput Benchmarks

These compare Symta's parser throughput against the C-port of
the reader (`runtime/reader.c`). The Symta-side `text.parse`
builtin is a thin trampoline into the C parser; when this
project's bootstrap (`sbc/reader.sbc`) is rebuilt against the
Symta-side parser these benches measure that path instead.

The 28× speedup in `architecture.md` was measured on
`parser-bench.s` -- the reader.s self-parse case.

## Files

| File | What it parses | Best for |
|------|----------------|----------|
| `parser-bench.s` | `symta/src/reader.s` (~14 KB, full parser source) | Sustained throughput |
| `parser-bench-small.s` | 5 example files, ~1-3 KB each | REPL / `-f` interactive cost |

## Running

```
# Build the runtime first if needed.
make runtime

# Substantial input (200 iterations).
./symta.exe -f benchmark/parser/parser-bench.s

# Small inputs (100 passes × 5 files).
./symta.exe -f benchmark/parser/parser-bench-small.s
```

Output is plain text -- read the numbers by eye. There's no
baseline file (the throughput varies with platform and parser
implementation choice).
