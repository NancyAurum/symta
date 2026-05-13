// tc_mixed — AM_GENERIC tables with mixed key types.
//
// AM-3 regression cover. dhEqual_ / dhHash_ now branch on
// O_TAG(key) and inline:
//   - T_INT        -> pointer equality / inlined Murmur3 finaliser
//   - T_FIXTEXT    -> pointer equality / 64-bit fold
//   - T_TEXT       -> memcmp / Adler-32
//   - mixed FIXTEXT/TEXT -> texts_equal helper
//   - other (lists / closures / tags) -> MCALL fallback
//
// The inlined hashes MUST agree bit-for-bit with what int.hash /
// text.hash return via MCALL. We verify by populating a generic
// table with each shape and round-tripping every key. A
// mis-aligned hash would make some keys invisible (the inline
// lookup probes the wrong bucket).

use cls

export tc_mixed

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_mixed =
  T (!)

  // Force GENERIC by writing a list key first.
  T.[zero] = \init

  // Int keys (cover positive, negative, large).
  T.0          = \zero
  T.1          = \one
  T.-1         = \neg_one
  T.999999     = \big
  T.-999999    = \neg_big
  check 'int 0'              \zero        T.0
  check 'int 1'              \one         T.1
  check 'int -1'             \neg_one     T.-1
  check 'int 999999'         \big         T.999999
  check 'int -999999'        \neg_big     T.-999999

  // Fixtext keys.
  T.\a    = \fixtext_a
  T.\hi   = \fixtext_hi
  T.\hello = \fixtext_hello   // 5 chars, still fixtext
  check 'fixtext a'          \fixtext_a     T.\a
  check 'fixtext hi'         \fixtext_hi    T.\hi
  check 'fixtext hello'      \fixtext_hello T.\hello

  // Bigtext keys (longer than the fixtext capacity).
  Long1 "this_is_definitely_a_bigtext_key"
  Long2 "another_long_key_for_the_table"
  T.Long1 = \bigtext_1
  T.Long2 = \bigtext_2
  check 'bigtext 1'          \bigtext_1   T.Long1
  check 'bigtext 2'          \bigtext_2   T.Long2

  // Tag-list keys (compound).
  T.[a b]    = \tag_pair
  T.[a b c]  = \tag_triple
  check 'tag pair'           \tag_pair    T.[a b]
  check 'tag triple'         \tag_triple  T.[a b c]

  // Round-trip every key via fresh lookup expressions (not the same
  // text cell as the one stored). For ints this is trivial (FXN(42)
  // is the same encoded value regardless of where it was written);
  // for text we want to make sure equivalent-content lookups hit.

  // Fresh text cell with the same content -- both fixtext...
  HiLookup "hi"   // string-literal "hi" -- fixtext.
  check 'fresh fixtext hit'  \fixtext_hi  T.HiLookup

  // ... and bigtext.
  LongLookup "this_is_definitely_a_bigtext_key"
  check 'fresh bigtext hit'  \bigtext_1   T.LongLookup

  // Iteration sanity.
  Count 0
  for [K V] T.l:
    when T.K >< V: Count = Count + 1
  check 'all keys reachable' T.n  Count

  // Delete a few keys, then re-iterate -- everything still
  // round-trips.
  T.del 0
  T.del \hello
  T.del Long2
  Count2 0
  for [K V] T.l:
    when T.K >< V: Count2 = Count2 + 1
  check 'after del, still ok' T.n Count2
  check 'has 0 after del'     0   (T.has 0)
  check 'has hello after del' 0   (T.has \hello)
