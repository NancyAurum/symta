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

## Conventions

- Each suite has its own `README.md` with running instructions.
- Output lines are plain `key: value` so they're easy to
  parse and diff. Avoid embedded ANSI escapes.
- Where applicable, commit a `baseline.txt` snapshot alongside
  any change that's expected to shift the numbers; reference
  the commit hash in the file's header.
- These are NOT regression tests. They don't run from
  `make test-all` and don't gate CI.
