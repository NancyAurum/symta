# Symta Benchmarks

Performance reference points, separated from the regression
tests in `tests/`. Benchmarks aren't pass/fail -- they print
timings you eyeball or diff between revisions.

## Suites

- `am/` -- adaptive map (hash table) ops across all modes.
  Has a committed `baseline.txt` for revision-to-revision
  comparison. The first benchmark-driven optimization
  (AM-15, dh hash cache) was sized using these numbers.
- `parser/` -- `text.parse` throughput. Compares the in-tree
  C parser (`runtime/reader.c`) against the Symta-side reader
  when rebuilt to use that path.
- `rt/` -- interpreter dispatch microbenchmarks (bn_loop,
  bn_call, bn_mcall, bn_list, bn_gc).  Single-opcode-in-a-tight-
  loop measurements; useful for spotting dispatch regressions.
- `game/` -- full Spell-of-Mastery rebuild (~22k lines,
  ~125 modules).  Real-world workload exercising the compiler,
  macroexpander, SIF assembler, and SBC loader end to end.
  Every commit that touches `runtime/` or `src/` should attach
  a fresh `[game cold]` / `[game warm]` line to its message.

## Conventions

- Each suite has its own `README.md` with running instructions.
- Output lines are plain `key: value` so they're easy to
  parse and diff. Avoid embedded ANSI escapes.
- Where applicable, commit a `baseline.txt` snapshot alongside
  any change that's expected to shift the numbers; reference
  the commit hash in the file's header.
- These are NOT regression tests. They don't run from
  `make test-all` and don't gate CI.
