// bn_iter -- iteration speed across modes.
//
// The .l, .ks, and `for K,V T:` ops all run through amL / amKs.
// Both eagerly snapshot the table into a fresh list, so the cost
// is N hash slots walked + N pairs allocated. Worth measuring
// because ECS systems iterate columns every frame.

export bn_iter

N      50000

report Label Elapsed =
  Per (Elapsed * 1000000000.0) / N.float
  say "[Label] [N]: [Elapsed] s, [Per.int] ns/entry"

bn_iter =
  say "\[bn_iter\]"
  say "# N=[N]"

  // ---- INT iteration ----
  TI (!)
  times I N: TI.I = (I + 1)

  T0 clock
  S 0
  for [_ V] TI.l: S = (S + V)
  T1 clock
  report "int_l " (T1 - T0)
  when S < 0: say "  (impossible)"

  T0 clock
  Ks TI.ks
  T1 clock
  report "int_ks" (T1 - T0)
  when Ks.n <> N: say "  (ks.n=[Ks.n])"

  // ---- TEXT iteration ----
  TT (!)
  TextN (N / 5)
  TextKeys (map I TextN: "k_[I]")
  times I TextN:
    K TextKeys[I]
    TT.K = I

  T0 clock
  S2 0
  for [_ V] TT.l: S2 = (S2 + V)
  T1 clock
  Per ((T1 - T0) * 1000000000.0) / TextN.float
  say "text_l [TextN]: [T1 - T0] s, [Per.int] ns/entry"
  when S2 < 0: say "  (impossible)"

  // ---- GENERIC iteration ----
  TG (!)
  GenN (N / 5)
  TG.[\sentinel] = \init
  GenKeys (map I GenN: [I I])
  times I GenN:
    K GenKeys[I]
    TG.K = (I + 1)

  T0 clock
  S3 0
  for [_ V] TG.l:
    when V.is_int: S3 = (S3 + V)
  T1 clock
  Per ((T1 - T0) * 1000000000.0) / GenN.float
  say "gen_l [GenN]: [T1 - T0] s, [Per.int] ns/entry"
  when S3 < 0: say "  (impossible)"

  // ---- BITMAP0 iteration via gid_refs_ ----
  TB (!)
  times I N: gid_set_ TB I 0

  T0 clock
  Refs (gid_refs_ TB 0)
  T1 clock
  report "b0_refs" (T1 - T0)
  when Refs.n <> N: say "  (refs.n=[Refs.n])"

  // ---- BITMAP1 iteration via .l ----
  TB1 (!)
  times I N: gid_set_ TB1 I 1

  T0 clock
  S4 0
  for [_ V] TB1.l: S4 = (S4 + V)
  T1 clock
  report "b1_l  " (T1 - T0)
  when S4 < 0: say "  (impossible)"

  say ""
