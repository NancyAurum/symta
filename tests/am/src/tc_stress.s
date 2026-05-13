// tc_stress — large maps, hash grow boundaries, mass mutation.
//
// Lots of dh/stb_ds bugs only show up when the table grows past
// its initial capacity and rehashes. This case fills four
// different-mode tables to ~10k entries, looks every key up,
// deletes half, looks up again, and verifies counts agree
// throughout. 10k is enough to cross multiple grow events while
// staying under a second on the test sweep.

use cls

export tc_stress

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

N 10000

stress_int =
  T (!)
  // Fill with int keys, non-trivial values.
  for I N: T.I = I*7
  check 'int N entries'      N  T.n

  // Random-access lookup: every key reads back its value.
  Bad 0
  for I N: when T.I <> I*7: Bad = Bad + 1
  check 'int N reads'        0  Bad

  // Delete every other key.
  for I N: when I -*- 1: T.del I
  check 'int N/2 after del'  N/2 T.n

  // Surviving keys still read back; deleted ones miss.
  Mismatch 0
  for I N:
    if I -*- 1
      then when (T.has I) <> 0:   Mismatch = Mismatch + 1
      else when T.I       <> I*7: Mismatch = Mismatch + 1
  check 'int post-del integrity' 0 Mismatch

stress_text =
  T (!)
  for I N:
    K "key_[I]"
    T.K = I
  check 'text N entries'     N  T.n

  Bad 0
  for I N:
    K "key_[I]"
    when T.K <> I: Bad = Bad + 1
  check 'text N reads'       0  Bad

stress_bitmap =
  T (!)
  for I N: T.I = 1   // -> AM_BITMAP1 the whole time
  check 'b1 N entries'       N  T.n

  Bad 0
  for I N: when T.I <> 1: Bad = Bad + 1
  check 'b1 N reads'         0  Bad

  // Promote to INT by writing one non-1.
  T.N = 42
  check 'b1->int after promote' 42 T.N
  check 'b1 keys survived'   1  T.0
  check 'b1->int n'          N+1 T.n

stress_generic =
  T (!)
  // Force GENERIC by writing a non-int, non-text key first. We
  // avoid `[0 0]` because the loop below also writes `[0 0]` and
  // would overwrite our sentinel, making the count off by one.
  T.[\sentinel] = \init
  // Then a few thousand list-keyed entries. Each list key is a
  // fresh allocation so dhEqual_ takes the list-equality MCALL
  // fallback path -- the hot loop for AM-3's "MCALL is the slow
  // path" claim.
  M 2000
  for I M: T.[I I] = I
  check 'generic M entries'  M+1  T.n

  Bad 0
  for I M:
    when T.[I I] <> I: Bad = Bad + 1
  check 'generic M reads'    0    Bad

tc_stress =
  stress_int
  stress_text
  stress_bitmap
  stress_generic
