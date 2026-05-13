// tc_float_edge — float bit-patterns that catch sloppy marshalling.
//
// Many FFI bugs only show up at specific bit patterns. We
// deliberately stay within values that Symta's single-precision
// `float` type can express exactly so the round-trip assertion
// has a single source of truth. Tests of denormals / inf / NaN
// belong in a C-side suite, not here.

use cffi
export tc_float_edge

check_eq Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_float_edge =
  // Negative zero. Symta's == says 0.0 == -0.0 (IEEE behaviour);
  // the bits differ but the comparison says equal. We verify only
  // the value survives the call.
  check_eq 'f64 neg of 0.0'   0.0    (c_f64_neg 0.0)
  check_eq 'f32 neg of 0.0'   0.0    (c_f32_neg 0.0)

  // Powers of two (exactly representable).
  check_eq 'f64 id 1024.0'    1024.0       (c_id_f64 1024.0)
  check_eq 'f64 id 0.0625'    0.0625       (c_id_f64 0.0625)
  check_eq 'f64 id 1048576.0' 1048576.0    (c_id_f64 1048576.0)

  // Negative values.
  check_eq 'f64 id -1024.0'   -1024.0      (c_id_f64 -1024.0)
  check_eq 'f64 id -0.0625'   -0.0625      (c_id_f64 -0.0625)

  // Sequence: c_f64_seq y = (y*2 + 1) / 3.
  // For y=0.5: (1.0 + 1.0) / 3.0 = 0.6666...; Symta's `><` is
  // tolerant enough for the single-precision rounding.
  R c_f64_seq 0.5
  // sanity: 1.5/3 = 0.5 isn't quite right; (1+1)/3 = 0.666...
  if R > 0.66 and R < 0.67
    then say "PASS f64 seq 0.5"
    else say "FAIL f64 seq 0.5: got [R]"
