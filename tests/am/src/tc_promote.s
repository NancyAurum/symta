// tc_promote — mode transitions.
//
// Coverage:
//   EMPTY    -> BITMAP0 (first write of value 0 on int key)
//   EMPTY    -> BITMAP1 (first write of value 1 on int key)
//   EMPTY    -> INT     (first write of !=0/1 value on int key)
//   EMPTY    -> TEXT    (first write of text key)
//   EMPTY    -> GENERIC (first write of list/closure/tag key)
//   BITMAP0  -> INT     (writing a non-0 value)
//   BITMAP1  -> INT     (writing a non-1 value)
//
// Each transition must preserve all previously inserted (key, value)
// pairs -- this is what the TODO calls out as the highest-risk
// part of the adaptive map.

use cls

export tc_promote

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

bitmap0_to_int =
  T (!)
  // Build a BITMAP0 with several 0 entries.
  for K [1 2 3 4 5 6 7 8]: T.K = 0
  check 'b0 n before promote' 8  T.n
  check 'b0 get 3 before'     0  T.3

  // Writing a non-0 value promotes to INT. Every previous 0 should
  // survive as a stored value.
  T.99 = 42
  check 'b0->int new key'     42 T.99
  for K [1 2 3 4 5 6 7 8]:
    check "b0->int preserve [K]" 0 T.K
  check 'b0->int n=9'         9  T.n

bitmap1_to_int =
  T (!)
  for K [10 20 30 40]: T.K = 1
  check 'b1 n before promote' 4  T.n
  check 'b1 get 30 before'    1  T.30

  T.50 = -7
  check 'b1->int new key'     -7 T.50
  for K [10 20 30 40]:
    check "b1->int preserve [K]" 1 T.K
  check 'b1->int n=5'         5  T.n

empty_to_each =
  // First-write-determines-mode. We can't introspect the mode
  // directly from Symta, but the observable read-back is enough --
  // each branch sets up an empty table, writes the trigger value,
  // and re-reads it.

  T0 (!)
  T0.5 = 0    // -> BITMAP0
  check 'EMPTY->BITMAP0' 0 T0.5

  T1 (!)
  T1.5 = 1    // -> BITMAP1
  check 'EMPTY->BITMAP1' 1 T1.5

  TI (!)
  TI.5 = 100  // -> INT
  check 'EMPTY->INT'     100 TI.5

  TT (!)
  TT.\key = \value  // -> TEXT
  check 'EMPTY->TEXT'    \value TT.\key

  TG (!)
  TG.[1 2 3] = \listval  // -> GENERIC
  check 'EMPTY->GENERIC' \listval TG.[1 2 3]

tc_promote =
  bitmap0_to_int
  bitmap1_to_int
  empty_to_each
