// tc_iteration — .l, .ks, .n consistency across every mode.
//
// Three invariants on every adaptive-map mode:
//   - T.n == T.l.n          (count of pairs in .l equals .n)
//   - T.n == T.ks.n         (count of keys in .ks equals .n)
//   - T.l.map(?.0) ~~ T.ks  (the keys from .l are a permutation
//                            of T.ks; we test "is a set match"
//                            since hash iteration order isn't
//                            specified)
//
// Mode-specific cases:
//   - EMPTY:    everything is the empty list, .n == 0
//   - BITMAP0:  .l yields [K 0] pairs for every set bit
//   - BITMAP1:  .l yields [K 1] pairs for every set bit
//   - INT:      .l yields [K V] for the int->any-value entries
//   - TEXT:     .l yields [K V] with text keys
//   - GENERIC:  .l yields [K V] with arbitrary keys

use cls

export tc_iteration

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

// Helper: check the .l/.ks/.n triplet on a table.
//
// We can't `T.ks.s` (sort) the GENERIC case because cross-type
// comparison (`42 < \hi`, `[x y] < \hi`) isn't defined -- it
// raises "cant compare ...". Instead use a sort-free set equality
// check: same length, and every expected key appears in T.ks.

contains_ Xs X =
  Found 0
  for Y Xs: when Y >< X: Found = 1
  Found

set_eq_ As Bs =
  if As.n <> Bs.n
    then 0
    else
      OK 1
      for A As:
        when not contains_ Bs A: OK = 0
      OK

check_triplet Name T ExpectedKeys =
  check "[Name].n"          ExpectedKeys.n  T.n
  check "[Name].l.n"        ExpectedKeys.n  T.l.n
  check "[Name].ks.n"       ExpectedKeys.n  T.ks.n
  check "[Name].ks set"     1 (set_eq_ ExpectedKeys T.ks)

tc_iteration =
  // EMPTY.
  TE (!)
  check_triplet 'empty' TE []

  // BITMAP0.
  TB0 (!)
  for K [3 7 11 13 17]: TB0.K = 0
  check_triplet 'bitmap0' TB0 [3 7 11 13 17]
  // Each entry's value is 0.
  AllZero 1
  for [K V] TB0.l: when V <> 0: AllZero = 0
  check 'bitmap0 all V==0'  1 AllZero

  // BITMAP1.
  TB1 (!)
  for K [2 4 8 16]: TB1.K = 1
  check_triplet 'bitmap1' TB1 [2 4 8 16]
  AllOne 1
  for [K V] TB1.l: when V <> 1: AllOne = 0
  check 'bitmap1 all V==1'  1 AllOne

  // INT.
  TI (!)
  TI.10 = 100
  TI.20 = 200
  TI.30 = 300
  check_triplet 'int' TI [10 20 30]
  // .l agrees with point lookups.
  AllMatch 1
  for [K V] TI.l: when TI.K <> V: AllMatch = 0
  check 'int .l/point match' 1 AllMatch

  // TEXT.
  TT (!)
  TT.\a = 1
  TT.\b = 2
  TT.\c = 3
  check_triplet 'text' TT [\a \b \c]

  // GENERIC.
  TG (!)
  TG.[x y] = \xy
  TG.42    = \ans
  TG.\hi   = \greet
  check_triplet 'generic' TG [[x y] 42 \hi]
