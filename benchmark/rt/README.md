# Runtime / Interpreter Benchmark Suite

Reference-point benchmarks for [`runtime/sbc.c`](../../runtime/sbc.c)
`sbc_exec_fn` — the bytecode dispatch loop. Each commit that
changes the interpreter, opcode handlers, or anything else on
the hot path should be accompanied by a refreshed baseline and
a note in the commit body about which numbers shifted.

The suite is **not** a regression test. Variance is non-trivial
(±5–10% run-to-run is normal); use the numbers as relative
reference points when evaluating a change.

## Groups

| File | What it measures |
|------|------------------|
| `bn_loop.s` | Raw dispatch overhead: `times I N: S = S + 1` and xor variant |
| `bn_arith.s` | Mixed integer arithmetic (FXNADD / FXNSUB / FXNMUL / FXNDIV / bit ops) in a branch-free hot loop |
| `bn_branch.s` | Branchy hot loops: alternating if/else and three-way `case` on `I % 3` |
| `bn_call.s` | Tiny Symta function call in a hot loop (CALL / SUBR / LEAVE / ARGLIST) |
| `bn_mcall.s` | Warm method-cache dispatch (MCALL with MCACHE_HIT every iteration) |
| `bn_list.s` | List index get/set + `.n` in a hot loop |

## Running

```
# All groups, printed as a table.
bash symta/benchmark/rt/run.sh

# One group.
bash symta/benchmark/rt/run.sh arith

# Save raw output for later comparison.
bash symta/benchmark/rt/run.sh --save before.txt
# ... make changes ...
bash symta/benchmark/rt/run.sh --save after.txt
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

## Tuning N

If a benchmark finishes too quickly (sub-millisecond) the float
clock loses precision. If it takes >5 s you'll get bored. Edit
`N` at the top of each `bn_*.s` to fit the machine; values are
tuned for a 2024-era laptop today (Intel i7-12700H class).

## Baselines

Two files in this directory record reference points:

- `baseline-switch.txt` — `switch(RD8)` dispatch (pre-RT-1).
- `baseline-threaded.txt` — computed-goto dispatch (post-RT-1).

The intent is for the active baseline to track the *current*
dispatch strategy; the historical one is kept alongside so
diff-able evidence of any future dispatch change survives in
the tree.

## What the numbers mean

A modern x86-64 core dispatches each bytecode in ~3–6 ns. The
fastest benchmark (`count` in `bn_loop`) executes the equivalent
of 3–4 SBC ops per loop iteration; expect ~12–15 ns/op for that
on a 2024-era laptop. Heavier ops (function call, method call,
list index) trail off proportionally to the per-opcode work.

## RT-1 result (May 2026)

The literature says computed-goto threaded dispatch beats
centralised `switch()` dispatch by 10–25 %. On this codebase,
on gcc 12.2.0 / Win64 / Intel i7-12700H, it was the other way
around: threaded was ~8 % *slower* on average. See
[`baseline-threaded.txt`](baseline-threaded.txt) for per-bench
numbers and a one-paragraph root cause.

The threaded path is still in the tree at
`#ifdef SBC_THREADED_DISPATCH`. Build it with:

```
make EXTRA_CFLAGS=-DSBC_THREADED_DISPATCH -f Makefile.w64
```

Both binaries pass all 9 test suites + the 3-round drift
test, so flipping the toggle is safe. The decision to keep
the default at switch dispatch is based on the measured
numbers, not on any theoretical assumption.
