// tc_bitmap0 — AM_BITMAP0 mode.
//
// Entered by writing FXN(0) as the first value on an empty table.
// The bitmap stores presence-only; reads on a "set" bit return 0,
// reads on an unset bit return the table's void value. The mode
// persists as long as every value written is 0; the first non-0
// value promotes to AM_INT (covered in tc_promote).
//
// Operations to cover here:
//   - Many sparse set
//   - Lookup hits and misses
//   - has / got
//   - del individual bits, then check has returns 0
//   - clear

use cls

export tc_bitmap0

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_bitmap0 =
  T (!)

  // First write of 0 -> AM_BITMAP0 mode (internal). Observable: T.42
  // is now 0, T.43 is still No.
  T.42 = 0
  check 'first write 0'      0   T.42
  check 'missing -> No'      No  T.43
  check 'has 42'             1   (T.has 42)
  check 'has 43'             0   (T.has 43)
  check 'n=1'                1   T.n

  // Add many keys. All still 0 -> stays in BITMAP0.
  for K [1 2 3 100 1000 10000]:
    T.K = 0
  check 'n=7 after adds'     7   T.n
  check 'get 100'            0   T.100
  check 'get 10000'          0   T.10000
  check 'get 9999 -> No'     No  T.9999

  // Sparse-but-large key still works (bitmap pages are
  // dynamically allocated; we shouldn't crash on a high bit).
  T.65535 = 0
  check 'get 65535'          0   T.65535
  check 'has 65535'          1   (T.has 65535)

  // del a key, has should return 0; the bitmap clears the bit.
  T.del 100
  check 'has 100 after del'  0   (T.has 100)
  check 'get 100 after del'  No  T.100
  check 'n after del'        7   T.n

  // del a never-set key is a no-op (the bit is already 0).
  T.del 99999
  check 'n after del-miss'   7   T.n

  // got vs has: a 0 value is present-but-No-equivalent? On a
  // BITMAP0 table, written-and-present returns FXN(0); got says
  // "present AND not No". 0 isn't No, so got should be 1.
  check 'got 42'             1   (T.got 42)

  // After clear we should be back to AM_EMPTY (n=0; everything
  // missing). The clear path frees the underlying nb_t.
  T.clear
  check 'n after clear'      0   T.n
  check 'has 42 after clear' 0   (T.has 42)
  check 'get 42 after clear' No  T.42
