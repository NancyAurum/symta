// tc_gid — `gid_get_` / `gid_set_` (the builtins around
// `amGidGet` / `amGidSet`).
//
// This is the AM-1 regression cover. amGidGet used to return its
// local `r` uninitialised for AM_GENERIC and AM_TEXT modes; the
// fix added explicit branches that (a) dispatch through `dhGet`
// for GENERIC (returning AM_VOID on miss) and (b) `fatal()` on
// AM_TEXT (a real contract violation that should crash loudly).
//
// We can't test the fatal path from inside Symta -- fatal() calls
// CRASH which terminates the process and isn't catchable by
// btrap. We exercise every NON-fatal AM mode through gid_get_:
//
//   - AM_EMPTY    -> void value
//   - AM_INT      -> stored value
//   - AM_BITMAP0  -> 0 if bit set, void otherwise
//   - AM_BITMAP1  -> 1 if bit set, void otherwise
//   - AM_GENERIC  -> stored value (post-AM-1 fix), void if missing

use cls

export tc_gid

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_gid =
  // For a plain int the GID is just the int value, so gid_set_(T, 7,
  // V) is equivalent to T.7 = V at the storage layer.

  // EMPTY: every gid_get_ returns the void.
  T0 (!)
  check 'gid EMPTY -> No'      No  (gid_get_ T0 42)

  T0b (!)
  T0b.setNo \void_v
  check 'gid EMPTY custom void' \void_v (gid_get_ T0b 99)

  // INT.
  TI (!)
  gid_set_ TI 5  100
  gid_set_ TI 10 200
  check 'gid INT 5'            100 (gid_get_ TI 5)
  check 'gid INT 10'           200 (gid_get_ TI 10)
  check 'gid INT missing -> No' No (gid_get_ TI 999)

  // BITMAP0.
  TB0 (!)
  for K [1 2 3 4 5]: gid_set_ TB0 K 0
  for K [1 2 3 4 5]:
    check "gid B0 [K]"         0   (gid_get_ TB0 K)
  check 'gid B0 missing -> No' No  (gid_get_ TB0 99)

  // BITMAP1.
  TB1 (!)
  for K [7 14 21]: gid_set_ TB1 K 1
  for K [7 14 21]:
    check "gid B1 [K]"         1   (gid_get_ TB1 K)
  check 'gid B1 missing -> No' No  (gid_get_ TB1 8)

  // GENERIC: built by writing a list key first. After that the
  // table is in AM_GENERIC; an int gid lookup still works because
  // the AM-1 fix routes it through dhGet.
  TG (!)
  TG.[1 2] = \pair
  TG.42    = \forty_two
  // gid_get_ uses an int GID. For GENERIC the key it builds is
  // FXN(42); the table stored an int-tagged 42 earlier so the
  // lookup hits.
  check 'gid GENERIC int hit'  \forty_two (gid_get_ TG 42)
  check 'gid GENERIC missing'  No         (gid_get_ TG 999)
