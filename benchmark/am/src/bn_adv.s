// bn_adv -- adaptive-map benchmarks under ADVERSARIAL key patterns.
//
// AM_INT mode is backed by ih.h (Murmur3 finaliser + Robin Hood).
// The Murmur3 finaliser is a bijection on 64 bits, so adversarial
// inputs CANNOT cluster its output in the upper bits -- but a weak
// hash function would.  These cases exercise key patterns that
// historically broke checksum-style hashes (Adler-32, naive
// xor-fold) to make sure ih.h doesn't regress to a weak hash if
// someone "simplifies" it.
//
// Each case populates N keys following the pattern, then measures
// hit / miss latency.  A weak hash function would show patterns
// running 5-10x slower than the `random` baseline.
//
// Run with `benchmark/am/run.sh adv`.

export bn_adv

N      50000
WARMUP 1000

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "  [Label] [N]: [Elapsed] s, [Per.int] ns/op"

// Stage a fresh table with the keys supplied by `make_key`.
populate Keyfn =
  T (!)
  times I N: T.(Keyfn I) = (I+1)
  T

// Time `n` lookups via Keyfn(I).
time_hit T Keyfn =
  Sum 0
  times I WARMUP: Sum = (Sum + T.(Keyfn I))
  T0 clock
  times I N: Sum = (Sum + T.(Keyfn I))
  T1 clock
  T1 - T0

// Time `n` lookups that miss (Keyfn shifted into never-populated range).
time_miss T Keyfn =
  Misses 0
  times I WARMUP:
    K (Keyfn I) + 100000000
    V T.K
    when V <> No: Misses = Misses + 1
  T0 clock
  times I N:
    K (Keyfn I) + 100000000
    V T.K
    when V <> No: Misses = Misses + 1
  T1 clock
  when Misses > 0: say "  (unexpected miss-hits: [Misses])"
  T1 - T0

run_case Label Keyfn =
  say "\[[Label]\]"
  T populate Keyfn
  H time_hit T Keyfn
  report "hit " H
  M time_miss T Keyfn
  report "miss" M
  say ""


// -----------------------------------------------------------------
// 1. Random-ish keys.  Linear congruential generator over the
//    int range -- good entropy across all bits.  Baseline.
// -----------------------------------------------------------------
bn_random =
  LCG_A 1664525
  LCG_C 1013904223
  run_case "random" (I => (I * LCG_A + LCG_C))

// -----------------------------------------------------------------
// 2. Sequential keys 0..N-1.  Common in array-style code.  A weak
//    low-bit hash would cluster these on the low cap bits.
// -----------------------------------------------------------------
bn_seq =
  run_case "sequential" (I => I)

// -----------------------------------------------------------------
// 3. Strided keys (aligned addresses).  All keys are multiples of
//    64 -- looks like cache-line-aligned heap pointers.  Low 6 bits
//    are constant.  A xor-fold hash would collide everything.
// -----------------------------------------------------------------
bn_stride64 =
  run_case "stride64" (I => I * 64)

// -----------------------------------------------------------------
// 4. Same-prefix keys.  High 32 bits identical, low 32 vary.  A
//    hash that folds via xor-of-halves would collapse them.
// -----------------------------------------------------------------
bn_same_prefix =
  Pref 0x123456000000
  run_case "same-prefix" (I => Pref -+- I)

// -----------------------------------------------------------------
// 5. Same-suffix keys.  Low 32 bits identical, high 32 vary.  A
//    hash that only looks at low bits collapses them.
// -----------------------------------------------------------------
bn_same_suffix =
  run_case "same-suffix" (I => (I -<- 32) -+- 12345)

// -----------------------------------------------------------------
// 6. Sparse powers of 2.  Only one bit set per key.  Catastrophic
//    for any hash that just masks the low bits.
// -----------------------------------------------------------------
bn_pow2 =
  // 50000 distinct values that are mostly powers-of-2-ish: we OR
  // with the index to keep them unique once we've burned through
  // bits 0..30.
  run_case "pow2-mostly" (I => 1 -<- (I -*- 31) -+- I)

// -----------------------------------------------------------------
// 7. "Adler killers": consecutive integers with the same low byte
//    sum modulo 65521.  Wouldn't trip Murmur3 but would trip
//    Adler-32-style hashes.
// -----------------------------------------------------------------
bn_adler_kill =
  // 65521 is the Adler-32 modulus.  Add it once per key so two
  // successive keys have the same Adler-32 value.
  run_case "adler-killer" (I => (I -*- 1) * 65521 + I)


bn_adv =
  say "# N=[N] warmup=[WARMUP]"
  bn_random
  bn_seq
  bn_stride64
  bn_same_prefix
  bn_same_suffix
  bn_pow2
  bn_adler_kill
