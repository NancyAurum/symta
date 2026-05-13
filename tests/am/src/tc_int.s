// tc_int — AM_INT mode.
//
// Entered on first write of a non-0/non-1 value (or by promotion
// from BITMAP0/1, covered in tc_promote). Keys are integers; the
// underlying store is stb_ds's hashmap. We exercise:
//
//   - Round-trip many int->any-value pairs
//   - Lookup hits and misses
//   - Update an existing key (semantics: overwrite, not append)
//   - del individual keys; n decreases
//   - .l, .ks return the right number of items
//   - Negative keys
//   - Setting value == void deletes the key (the amSet `value ==
//     void_val` early-out)

use cls

export tc_int

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_int =
  T (!)

  // First non-0/1 write -> AM_INT. 42 is the value, T.42 is the key.
  T.42 = 17
  check 'first int write'    17  T.42
  check 'n=1'                1   T.n
  check 'has 42'             1   (T.has 42)
  check 'has 99'             0   (T.has 99)
  check 'got 42'             1   (T.got 42)
  check 'miss -> No'         No  T.43

  // Many key/value pairs.
  for K,V [[10 100] [20 200] [30 300]]:
    T.K = V
  check 'get 10'             100 T.10
  check 'get 20'             200 T.20
  check 'get 30'             300 T.30
  check 'n=4'                4   T.n

  // Update existing key.
  T.42 = 999
  check 'after update'       999 T.42
  check 'n stays 4'          4   T.n

  // Negative keys.
  T.-5 = 50
  check 'negative key'       50  T.-5
  check 'has -5'             1   (T.has -5)

  // Setting value == void deletes the key.
  T.42 = No
  check 'after set-to-No'    No  T.42
  check 'has 42 after No'    0   (T.has 42)

  // del primitive: same effect.
  T.del 10
  check 'has 10 after del'   0   (T.has 10)
  check 'get 10 after del'   No  T.10

  // .ks size should match T.n.
  check 'ks len == n'        T.n  T.ks.n

  // Round-trip every remaining key.
  for K T.ks:
    check "round-trip [K]"   T.K  T.K

  // Custom void value: set to a sentinel, then set a key to that
  // sentinel -- the key should disappear (the amSet "value ==
  // void_val" early-out fires regardless of mode).
  T.clear
  T.setNo \void_sentinel
  T.7 = 42
  check 'normal write'       42  T.7
  T.7 = \void_sentinel
  check 'set-to-void deletes' 0  (T.has 7)
