// tc_double — double-precision round-trip and arithmetic.
//
// Doubles use the full 64 of an xmm slot. Any high-32 corruption
// (an FFI bug that splits the slot) shows up here as wildly
// wrong magnitudes.

use cffi
export tc_double

check_near Label Expected Got =
  Diff Expected - Got
  if Diff.abs.float < 0.000001  // 1e-6; loose tolerance avoids compiler %.8f precision loss -- see TODO READER-1
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_double =
  check_near 'f64 id 0.0'         0.0      (c_id_f64 0.0)
  check_near 'f64 id 1.0'         1.0      (c_id_f64 1.0)
  check_near 'f64 id 1e15'        1.0e15   (c_id_f64 1.0e15)
  check_near 'f64 id 1e-15'       1.0e-15  (c_id_f64 1.0e-15)
  check_near 'f64 id -1.5'        -1.5     (c_id_f64 -1.5)

  // High-precision pi — would be 3.14 if low/high halves swapped.
  check_near 'f64_pi const'       3.141592653589793 (c_const_f64_pi())

  // Two-arg double add.
  check_near 'f64 add 1.5+2.25'   3.75     (c_add_f64_f64 1.5 2.25)
  check_near 'f64 add 1e10+1e10'  2.0e10   (c_add_f64_f64 1.0e10 1.0e10)

  // Negation and the canonical f64_seq transformation:
  //   y = ((x*2 + 1) / 3)
  check_near 'f64 neg 1.5'        -1.5     (c_f64_neg 1.5)
  check_near 'f64 seq 4.0'        3.0      (c_f64_seq 4.0)
  // (4.0 * 2 + 1) / 3 = 9.0 / 3 = 3.0
