// tc_text — AM_TEXT mode.
//
// Entered on first write of a TEXT-keyed entry on an empty table.
// We cover:
//   - Short keys (fixtext) and long keys (bigtext) both lookup
//     correctly. The underlying store is stb_ds's `sh*` shashmap.
//   - Lookup with a key that has the same content but a different
//     representation (fixtext vs bigtext for an 8-char-ish boundary)
//     still hits.
//   - has / got / del work
//   - Non-text key on a TEXT table returns the void value, doesn't
//     blow up

use cls

export tc_text

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_text =
  T (!)

  T.\name = \Nancy
  check 'first text write'   \Nancy  T.\name
  check 'n=1'                1       T.n
  check 'has name'           1       (T.has \name)
  check 'has missing'        0       (T.has \other)
  check 'miss -> No'         No      T.\other

  // Several keys.
  T.\age   = 37
  T.\city  = \Amsterdam
  T.\stack = "lisp,haskell,c"
  check 'get age'            37            T.\age
  check 'get city'           \Amsterdam    T.\city
  check 'get stack'          "lisp,haskell,c" T.\stack
  check 'n=4'                4             T.n

  // Update.
  T.\age = 38
  check 'updated age'        38  T.\age
  check 'n stays 4'          4   T.n

  // Long key (bigtext): exceeds fixtext capacity, lives on heap.
  Long "this_is_a_long_key_that_wont_fit_in_a_fixtext"
  T.Long = \value
  check 'long-key write'     \value  T.Long
  check 'long-key has'       1       (T.has Long)

  // Looking up an int key on a TEXT table -- not a programming error,
  // just a "no such key" result. The table doesn't promote (would be
  // a mode change); it returns void.
  check 'int on TEXT -> No'  No      T.42
  check 'has int on TEXT'    0       (T.has 42)

  // del.
  T.del \city
  check 'has city after del' 0   (T.has \city)
  check 'get city after del' No  T.\city
  check 'n after del'        4   T.n

  // del-miss.
  T.del \nonexistent
  check 'n after del-miss'   4   T.n
