// tc_float — single-precision float round-trip and arithmetic.
//
// FFI floats: passed in the low 32 of an xmm slot, returned in
// the low 32 of xmm0. If the dispatcher confuses single-vs-
// double bit layouts these tests fail.

use cffi
export tc_float

check_near Label Expected Got =
  Diff Expected - Got
  // float roundtrip should be exact for representable values.
  if Diff.abs.float < 0.0001
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_float =
  // Identity on representable singles.
  check_near 'f32 id 0.0'    0.0    (c_id_f32 0.0)
  check_near 'f32 id 1.0'    1.0    (c_id_f32 1.0)
  check_near 'f32 id 2.5'    2.5    (c_id_f32 2.5)
  check_near 'f32 id -2.5'   -2.5   (c_id_f32 -2.5)
  check_near 'f32 id 100.5'  100.5  (c_id_f32 100.5)
  check_near 'f32 id 1e6'    1000000.0  (c_id_f32 1000000.0)

  // The float pi constant. 3.14159274 is the f32 representation
  // of math pi (closest IEEE single).
  check_near 'f32_pi const'  3.14159274  (c_const_f32_pi())

  // Two-arg arithmetic (exercises arg passing of multiple floats).
  check_near 'f32 add 1+2'   3.0    (c_add_f32_f32 1.0 2.0)
  check_near 'f32 add .5+.25' 0.75  (c_add_f32_f32 0.5 0.25)

  // Negation.
  check_near 'f32 neg 1.5'   -1.5   (c_f32_neg 1.5)
  check_near 'f32 neg -3.0'  3.0    (c_f32_neg -3.0)
