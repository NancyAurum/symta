// tc_arith — 2-arg arithmetic across all type combos.
//
// Catches "the second argument got the wrong slot" or "the
// type-pool wasn't classified correctly". Each function is
// associative so we just check a single (a, b) pair per type.

use cffi
export tc_arith

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

check_near Label Expected Got =
  Diff Expected - Got
  if Diff.abs.float < 1e-9
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_arith =
  // Pure int + int.
  check       'i32 add 7+5'      12       (c_add_i32_i32 7 5)
  check       'i32 add -3+10'    7        (c_add_i32_i32 -3 10)

  // Pure u32 + u32.
  check       'u32 add 100+200'  300      (c_add_u32_u32 100 200)
  check       'u32 add 0xFF+1'   256      (c_add_u32_u32 0xFF 1)

  // Pure float + float.
  check_near  'f32 add 1.5+2.5'  4.0      (c_add_f32_f32 1.5 2.5)

  // Pure double + double.
  check_near  'f64 add 1e3+1e3'  2000.0   (c_add_f64_f64 1.0e3 1.0e3)

  // Mixed (int, double) — exercises argument heterogeneity.
  // On SysV the int goes to rdi while the double goes to xmm0.
  // On Win64 the int goes to rcx and the double goes to xmm1
  // (slot-position-1).
  check_near  'mix i32+f64'      8.5      (c_add_i32_f64 3 5.5)
  check_near  'mix f64+i32'      9.25     (c_add_f64_i32 4.25 5)
