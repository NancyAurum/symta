// bn_promote -- mode-transition cost.
//
// Each promotion (BITMAP0 -> INT, BITMAP1 -> INT, BITMAP -> GENERIC,
// INT -> GENERIC, TEXT -> GENERIC) walks every existing entry and
// migrates it to the new backing. The cost is roughly N hash
// inserts on the new structure, plus the overhead of allocating
// the new structure and freeing the old.
//
// We don't try to time a single transition (too noisy for clock()).
// Instead we run M transitions and divide.

export bn_promote

POP    1000     // entries in the table at promotion time
ROUNDS 200      // number of promotion cycles per measurement

report Label Elapsed =
  Total (POP * ROUNDS)
  Per (Elapsed * 1000000000.0) / Total.float
  say "[Label] pop=[POP] rounds=[ROUNDS]: [Elapsed] s, [Per.int] ns/entry"

bn_promote =
  say "\[bn_promote\]"

  // ---- BITMAP0 -> INT ----
  T0 clock
  times R ROUNDS:
    T (!)
    times I POP: gid_set_ T I 0
    gid_set_ T POP 42
  T1 clock
  report "b0->int" (T1 - T0)

  // ---- BITMAP1 -> INT ----
  T0 clock
  times R ROUNDS:
    T (!)
    times I POP: gid_set_ T I 1
    gid_set_ T POP 42
  T1 clock
  report "b1->int" (T1 - T0)

  // ---- BITMAP0 -> GENERIC ----
  T0 clock
  times R ROUNDS:
    T (!)
    times I POP: gid_set_ T I 0
    T.\flag = 1
  T1 clock
  report "b0->gen" (T1 - T0)

  // ---- INT -> GENERIC ----
  T0 clock
  times R ROUNDS:
    T (!)
    times I POP: T.I = (I * 2)
    T.\flag = 1
  T1 clock
  report "int->gen" (T1 - T0)

  // ---- TEXT -> GENERIC ----
  TextPop (POP / 5)
  TextRounds (ROUNDS * 5)
  T0 clock
  times R TextRounds:
    T (!)
    times I TextPop:
      K "k_[I]"
      T.K = I
    T.42 = 99
  T1 clock
  TotalEntries (TextPop * TextRounds)
  Per ((T1 - T0) * 1000000000.0) / TotalEntries.float
  say "text->gen pop=[TextPop] rounds=[TextRounds]: [T1 - T0] s, [Per.int] ns/entry"

  say ""
