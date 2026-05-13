// bn_bitmap -- AM_BITMAP0 / AM_BITMAP1 benchmarks.
//
// Bitmap mode is used by cls field columns whose value
// distribution is almost-always 0 (BITMAP0) or 1 (BITMAP1) --
// e.g. "is_alive", "is_visible". gid_set_ / gid_get_ are the
// component-column setters; they go through amGidSet /
// amGidGet which prefer the bitmap representation over INT
// for these specific values.
//
// Bitmap pages are 512 bytes each (8 × 64-bit words = 4096
// bits). At dense populations the per-op cost is dominated by
// the nh_t lookup for the page key, which is cheap (uint64_t
// hash, linear probing, since AM-5 left nb.h on plain LP).

export bn_bitmap

N      100000
WARMUP 1000

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/op"

bn_bitmap =
  say "\[bn_bitmap\]"
  say "# N=[N] warmup=[WARMUP]"

  // ---- BITMAP0 SET ----
  T (!)
  times I WARMUP: gid_set_ T I 0
  T.clear

  T0 clock
  times I N: gid_set_ T I 0
  T1 clock
  report "b0_set" (T1 - T0)

  // ---- BITMAP0 GET ----
  Sum 0
  times I WARMUP: Sum = (Sum + (gid_get_ T I))

  T0 clock
  times I N: Sum = (Sum + (gid_get_ T I))
  T1 clock
  report "b0_get" (T1 - T0)
  when Sum < 0: say "  (impossible: sum=[Sum])"

  // ---- BITMAP1 SET ----
  T2 (!)
  times I WARMUP: gid_set_ T2 I 1
  T2.clear

  T0 clock
  times I N: gid_set_ T2 I 1
  T1 clock
  report "b1_set" (T1 - T0)

  // ---- BITMAP1 GET ----
  Sum2 0
  times I WARMUP: Sum2 = (Sum2 + (gid_get_ T2 I))

  T0 clock
  times I N: Sum2 = (Sum2 + (gid_get_ T2 I))
  T1 clock
  report "b1_get" (T1 - T0)
  when Sum2 < 0: say "  (impossible: sum=[Sum2])"

  // ---- gid_refs_: list of refs (entity-ID-tagged) from the
  // bitmap. The dominant cost is the NB_FOR walk over all set
  // bits.
  T0 clock
  Refs (gid_refs_ T 0)
  T1 clock
  report "b0_refs" (T1 - T0)
  when Refs.n <> N: say "  (refs.n=[Refs.n], expected [N])"

  say ""
