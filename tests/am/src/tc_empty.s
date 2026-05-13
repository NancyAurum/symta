// tc_empty — empty-table semantics and void value handling.
//
// A freshly-allocated table is in AM_EMPTY mode. Its observable
// behaviour:
//   - .n is 0
//   - .has K returns 0 for every K
//   - .got K returns 0 for every K
//   - .l is the empty list
//   - .ks is the empty list
//   - T.K returns the table's void value (`No` by default)
//   - del K is a no-op (safe)
//   - .setNo V changes the void value retroactively
//
// AM_EMPTY -> any-other-mode transition is covered in tc_promote.

use cls

export tc_empty

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_empty =
  T (!)
  check 'is_table'           1 T.is_table
  check 'fresh n == 0'       0 T.n
  check 'fresh l == ()'      [] T.l
  check 'fresh ks == ()'     [] T.ks

  check 'has missing int'    0 (T.has 42)
  check 'has missing text'   0 (T.has \foo)
  check 'has missing list'   0 (T.has [1 2])
  check 'got missing int'    0 (T.got 42)

  check 'get missing -> No'  No T.42
  check 'get text -> No'     No T.\foo

  // del on a missing key is silent
  T.del 99
  check 'n after del-miss'   0 T.n

  // Custom void value via .setNo. Subsequent gets for missing keys
  // should return the new void value, NOT the old No.
  T.setNo \void_sentinel
  check 'get after setNo'    \void_sentinel  T.99

  // .clear resets the table; nothing crashes if it was already empty.
  T.clear
  check 'n after clear'      0 T.n
