# Game compilation benchmark

Times a full and an incremental rebuild of the Spell of Mastery
game source (`../game/`, ~22k lines of Symta across ~125
modules).  Real-world workload for the compiler / macroexpander /
SIF assembler -- the synthetic [bn_* benchmarks](../rt/) measure
single-opcode dispatch in tight loops; this one measures the
end-to-end pipeline on a project an order of magnitude larger.

## When to run it

Every commit that touches `runtime/` (interpreter, sif assembler,
sbc layout, GC) **or** `src/` (compiler, macros, reader) should
attach a fresh game-benchmark line in the commit message.  Format:

```
[game cold]  N.NN s   123 modules
[game warm]  N.NNN s   0 modules
```

Variance is real on Windows (Defender real-time scan, cold FS):
±2-4 s cold, ±20-40 ms warm.  Treat as bands, not point values.

## Running

```sh
bash benchmark/game/run.sh              # cold + warm
bash benchmark/game/run.sh --warm-only  # skip the cold wipe
bash benchmark/game/run.sh --save x.txt # tee raw output
```

## What's measured

- **cold**: deletes `../game/sbc/` and `../game/cache/` so every
  module recompiles from source.  Dominated by macroexpansion +
  SSA + sif-to-sbc throughput.
- **warm**: re-runs the build on top of an existing cache.
  Should be ~0.15 s -- it just sbc-loads every module, dep-walks,
  and finds nothing stale.  Useful as a smoke test for any change
  that touches SBC loading or dep tracking.

## Baselines

Each substantial perf-affecting commit drops a baseline file next
to this README, named `baseline-<commit-tag>.txt`.

| Commit | Tag | Cold | Warm | Notes |
|--------|-----|-----:|-----:|-------|
| 0b9e2c4 | post-core1-sidetable | ~52 s | ~0.15 s | CORE-1 v2 (lineno side table) |
