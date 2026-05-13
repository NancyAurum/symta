// tc_bitmap1 — AM_BITMAP1 mode.
//
// Symmetric to tc_bitmap0. Entered by writing FXN(1) on an empty
// table; reads on a "set" bit return 1.

use cls

export tc_bitmap1

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_bitmap1 =
  T (!)

  T.7 = 1
  check 'first write 1'      1   T.7
  check 'missing -> No'      No  T.8
  check 'has 7'              1   (T.has 7)
  check 'has 8'              0   (T.has 8)
  check 'n=1'                1   T.n

  // More 1s -- stays in BITMAP1.
  for K [11 13 17 19 23 29]:
    T.K = 1
  check 'n=7 after primes'   7   T.n
  for K [11 13 17 19 23 29]:
    check "get [K]"          1   T.K

  // High-numbered bit shouldn't crash.
  T.123456 = 1
  check 'get 123456'         1   T.123456
  check 'n after big bit'    8   T.n

  // del clears, has returns 0.
  T.del 13
  check 'has 13 after del'   0   (T.has 13)
  check 'get 13 after del'   No  T.13
  check 'n after del'        7   T.n

  // del-miss is silent.
  T.del 12345678
  check 'n after del-miss'   7   T.n

  // got: 1 is not No -> got returns 1.
  check 'got 7'              1   (T.got 7)

  // clear empties.
  T.clear
  check 'n after clear'      0   T.n
  check 'has 7 after clear'  0   (T.has 7)
