// tc_generic — AM_GENERIC mode.
//
// Entered when a key is anything other than a plain int or plain
// text (lists, closures, tags, mixed-type keys, ...). The underlying
// store is `dh_t` -- the runtime/dh.h hashmap that dispatches via
// MCALL by default, with AM-3's inline fast paths for int/text.

use cls

export tc_generic

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_generic =
  T (!)

  // First write of a list key -> AM_GENERIC mode (or maybe the
  // table goes through INT->GENERIC promotion -- either way the
  // observable behaviour below should hold).
  T.[1 2] = \pair
  check 'list-key write'     \pair   T.[1 2]
  check 'has [1 2]'          1       (T.has [1 2])
  check 'has [1 3]'          0       (T.has [1 3])
  check 'n=1'                1       T.n

  // After GENERIC, every type of key works.
  T.42       = \forty_two
  T.\hello   = \greeting
  T.[a b c]  = \abc
  T.[7 7]    = \double_seven
  check 'get int key'        \forty_two    T.42
  check 'get text key'       \greeting     T.\hello
  check 'get tag list key'   \abc          T.[a b c]
  check 'get [7 7]'          \double_seven T.[7 7]
  check 'n=5'                5             T.n

  // Lookups of structurally equal but distinct-cell lists should hit
  // (lists compare by content, hash by content). This exercises the
  // dh equality path; under AM-3 the int and text fast paths handle
  // those keys inline, but list keys still take the MCALL fallback.
  K1 [1 2]
  K2 [1 2]
  // K1 and K2 are distinct cells but `K1 >< K2` should hold.
  check 'list equality'      1       (K1 >< K2)
  check 'lookup via fresh key' \pair T.K2

  // del a generic key.
  T.del [1 2]
  check 'has [1 2] after del' 0   (T.has [1 2])
  check 'n after del'         4   T.n

  // del a missing generic key.
  T.del [99 99]
  check 'n after del-miss'    4   T.n

  // Update.
  T.\hello = \GREETING
  check 'updated text in gen' \GREETING T.\hello

  // Iteration: every key should be readable.
  Count 0
  for [K V] T.l:
    when T.K >< V: Count = Count + 1
  check 'iter agrees with l'  T.n  Count
