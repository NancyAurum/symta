// tc_void — `void_val` contract and the "write-void deletes" rule.
//
// `AM_VOID(o)` (a.k.a. T.setNo's value) is what amGet returns when
// a key isn't present. By default it's `No`. amSet/amGidSet
// interpret `value == void_val` as a delete request. Documented
// in the am.h header block; pinned here so changes that violate
// the contract show up.
//
// Things to cover:
//   - default void_val is No
//   - T.setNo X changes the void_val
//   - writing void_val to a key is the delete branch
//   - writing the default-void (No) deletes regardless of mode
//   - writing a *custom* void_val deletes; writing 0 still works
//     as a real value when void_val is something else
//   - this works through every AM_* mode: AM_EMPTY, AM_INT,
//     AM_TEXT, AM_GENERIC, AM_BITMAP*

use cls

export tc_void

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_void =
  // ---- default void_val (No) ----
  T (!)
  check 'default void missing'   No  T.42
  T.42 = 100
  check 'after write get back'   100 T.42
  // Writing No to a present key is a delete -- because No is the
  // default void_val.
  T.42 = No
  check 'write No deletes'       0   (T.has 42)
  check 'after No-delete get'    No  T.42

  // ---- custom void_val on INT ----
  TC (!)
  TC.setNo \empty_marker
  check 'TC custom void missing' \empty_marker  TC.42
  TC.42 = 100
  check 'TC write real value'    100 TC.42
  // Writing the void marker deletes.
  TC.42 = \empty_marker
  check 'TC write void deletes'  0   (TC.has 42)
  // Writing 0 stores 0 (because void_val is no longer 0).
  TC.42 = 0
  check 'TC write 0'             0   TC.42
  check 'TC has 42 after 0'      1   (TC.has 42)
  // Writing No (no longer the void) just stores No as a value.
  TC.99 = No
  check 'TC No is a real value'  No  TC.99
  // Wait -- amGet returns AM_VOID(o) on miss too. We can't
  // distinguish "stored No" from "missing" via direct lookup
  // alone. .has is the discriminator.
  check 'TC has 99 (stored No)'  1   (TC.has 99)

  // ---- void_val = 0: the AM-7 quirk ----
  // This is the case the doc block calls out. After setNo 0, a
  // user writing T.K = 0 looks like a write but is actually a
  // delete. We pin the surprising behaviour so future refactors
  // don't accidentally fix it without updating the doc.
  TZ (!)
  TZ.setNo 0
  TZ.5 = 100
  check 'TZ has 5'               1   (TZ.has 5)
  TZ.5 = 0    // intended-write that's actually a delete
  check 'TZ has 5 after 0-write' 0   (TZ.has 5)
  check 'TZ get returns void'    0   TZ.5

  // ---- TEXT mode void ----
  TT (!)
  TT.\a = 1
  TT.\b = 2
  TT.\c = 3
  TT.\b = No   // delete via void
  check 'TEXT delete via void'   0   (TT.has \b)
  check 'TEXT survivors'         3   TT.\c

  // ---- GENERIC mode void ----
  // Build a generic table by writing a non-int, non-text key
  // (a list). Then delete via void.
  TG (!)
  TG.[1 2] = 100
  TG.[3 4] = 200
  TG.[1 2] = No
  check 'GENERIC delete via void' 0  (TG.has [1 2])
  check 'GENERIC survivor'        200 TG.[3 4]

  // ---- BITMAP1 via amSet ----
  // T.K = 1 on an empty table enters BITMAP1. Then T.K = No
  // should delete because No is the void.
  TB (!)
  TB.5 = 1
  TB.7 = 1
  TB.5 = No
  check 'BITMAP1 delete via No'   0  (TB.has 5)
  check 'BITMAP1 survivor'        1  TB.7
