# Weak hashtable perf investigation

## Question

The reader consolidation's first attempt (commits `3229299` … `cae94d5`)
backed `meta` on a global weak hashtable.  Game cold compile went
51-54 s → 93-95 s, +75 %.  Reverting to a wrapper-meta type
(commit `2129767`) brought cold compile to 38-40 s -- ~25 % BETTER
than baseline thanks to the C reader.

The natural question: "Interpreted code just can't be faster than
C.  Why was the hashtable so slow?"

## Micro-benchmark results (commit `8f8ff63`)

Loop body cost on a populated table (50 000 entries):

| body | ns/op | notes |
|---|---|---|
| `times I N: I` | 7 | loop overhead floor |
| `times I N: S = S+1` | 12 | + int add |
| `Keys.I` (list subscript) | 10 | list `.` builtin |
| `wh_get_(K)` same key, hot cache | 15 | best case, slot stays in L1 |
| `wh_get_(Keys.I)`, populated table | 190 | real-world miss-mostly cost |
| `V <> No` standalone, V is a list | 256 | `<>` dispatch chain |
| `got V` (after `wh_get_`) | +5 | very cheap |

So the inherent hashtable lookup cost is ~190 ns.  Breakdown:

  ~50 ns   MCALL `wh_get_` dispatch (method resolution + frame setup)
  ~10 ns   C call into `meta_get_`
  ~10 ns   IMMEDIATE check + table-null check
 ~100 ns   `ihGet` (Murmur3 finaliser + Robin Hood probe + cache miss)
  ~20 ns   return path + Symta-side binding

Compare to the wrapper-meta type's `.meta_` field access:

  ~30 ns   MCALL `.meta_` dispatch into the type's auto-generated
           field accessor
  ~5 ns    LGET of slot 1

Wrapper-meta is **~5-6× faster per call** than the hashtable.

## Why the multiplier matters

A 21 k-LOC game compile reads `.meta_` and `.is_meta` on roughly
every AST node visited by the compiler and macroexpander -- on the
order of 200-300 million calls.  At 160 ns/call extra cost, that's
30-50 seconds of compile time eaten by the dispatch + lookup.
Matches the +40-50 s regression measured on the game bench.

## Is the hash function broken on adversarial keys?

No.  ih.h hashes the raw 64-bit dyn key via the Murmur3 finaliser
(`ihFmix64_`):

```c
INLINE uint64_t ihFmix64_(uint64_t key) {
  key ^= key >> 33;
  key *= 0xff51afd7ed558ccdULL;
  key ^= key >> 33;
  key *= 0xc4ceb9fe1a85ec53ULL;
  key ^= key >> 33;
  return key;
}
```

This is a bijection (it's the standard Murmur3 finaliser, designed
for exactly this purpose).  Every input bit reaches every output
bit, so no adversarial input pattern produces clustered output.

For heap-pointer keys the dyn is `(gid << 16) | tag_bits | T_HEAP`.
The tag bits are constant within a type, but Murmur3 mixes them
into the high bits of the hash where the cap mask doesn't reach.
Distribution is uniform.

## Where the regression came from

Not a hash issue, not a probing issue.  It's the fundamental
indirection cost of a hashtable lookup vs a field load.  Every
`.meta_` access:

  - wrapper: 1 cache line (the wrapper struct itself)
  - hashtable: 1-3 cache lines (probe sequence in the table's
    `slots` array, which is much larger than L1)

The probe is L3-bound at ~100 ns; the wrapper field is L1-bound at
~5 ns.  No amount of hash-function tuning closes that gap.

## What this confirms

  - Switch decision was right (commit `2129767`): use wrapper-meta
    for the parser, not the weak table.
  - The weak hashtable infrastructure is still useful for code that
    needs identity-keyed metadata WITHOUT the per-access cost being
    on a hot loop (e.g., debug info, occasional inspector queries).
  - The `<> No` idiom (`when V <> No: ...`) is ~250 ns/call because
    it dispatches `<>` then `>< ` then negates.  In hot loops,
    prefer `when got V:` (~5 ns) or `when V:` (truthy check, ~3 ns).

## Followups worth considering

  - Symta-level: rewrite hot `.meta_` consumers to use `got X.meta_`
    instead of `X.meta_ <> No`.  Free perf win regardless of
    hashtable vs wrapper.
  - Runtime-level: inline a fast path for `<>` and `><` when one
    operand is the literal `No`, dispatched at the SBC level (no
    method call).  Affects code generation, not the hash table.
  - Compiler-level: constant-fold `X <> No` to `not (X is No)`
    where `is No` is a single-tag comparison.

None of these are blocking; the wrapper-meta path is fast enough.
This doc just rules out "hashtable is buggy" as the explanation.
