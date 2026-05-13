# Adaptive-Map Benchmark Suite

Reference-point benchmarks for `runtime/am.h` (the adaptive
hash-table backing every Symta `T_TBL`). Each commit that
changes the AM should be accompanied by a refreshed baseline
and a note in the commit body about which numbers shifted.

The suite is **not** a regression test. Variance is non-trivial
(±5–10% run-to-run is normal); use the numbers as relative
reference points when evaluating a change.

## Groups

| File | What it measures |
|------|------------------|
| `bn_int.s` | AM_INT (ih_t since AM-6): insert / hit / miss / del / iter |
| `bn_text.s` | AM_TEXT (still stb_ds; see AM-6b): insert / hit / miss / del / iter |
| `bn_generic.s` | AM_GENERIC (dh_t, list keys → MCALL fallback): same op set |
| `bn_bitmap.s` | AM_BITMAP0/1 via `gid_set_` / `gid_get_` / `gid_refs_` |
| `bn_promote.s` | Mode-transition cost (BITMAP→INT, BITMAP→GENERIC, etc.) |
| `bn_iter.s` | `.l` / `.ks` / `gid_refs_` across all modes |

## Running

```
# All groups, printed as a table.
bash symta/benchmark/am/run.sh

# One group.
bash symta/benchmark/am/run.sh int

# Save raw output for later comparison.
bash symta/benchmark/am/run.sh --save before.txt
# ... make changes ...
bash symta/benchmark/am/run.sh --save after.txt
diff -u before.txt after.txt
```

The harness builds its `go.exe` from `src/` the first time it
runs; subsequent runs reuse the build.

## Output format

Each measurement line is:

```
<op> <N>: <elapsed> s, <ns_per_op> ns/op
```

- `<N>` is the number of operations measured (each measurement
  has a configurable warmup phase whose cost isn't counted).
- `<elapsed>` is wall-clock seconds (via `clock()`).
- `<ns_per_op>` is `1e9 * elapsed / N`.

Promotion benchmarks report ns per migrated entry rather than
ns per op:

```
<transition> pop=<P> rounds=<R>: <elapsed> s, <ns_per_entry> ns/entry
```

## Tuning N

If a benchmark finishes too quickly (sub-millisecond) the float
clock loses precision. If it takes >5 s you'll get bored. Edit
`N` at the top of each `bn_*.s` to fit the machine; values are
tuned for a 2024-era laptop today.

## Baseline

`baseline.txt` is the current revision's output for reference.
It's committed alongside changes that move the numbers; check
the commit history for which revision a particular baseline
matches.

```
git log --oneline -- symta/benchmark/am/baseline.txt
```
