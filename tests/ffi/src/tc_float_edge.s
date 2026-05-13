// tc_float_edge — float bit-patterns that catch sloppy marshalling.
//
// Many FFI bugs only show up at specific bit patterns. A naive
// "treat float as int and pad" works for most normal values but
// breaks for NaN, infinity, negative zero, denormals. We don't
// require any of these to survive perfectly (some are
// platform-dependent), but the more we can pin down the better.

use cffi
export tc_float_edge

check_eq_bits Label Expected Got =
  // Symta doesn't have a portable "float bitcast" yet, so we
  // compare directly. For values where Symta == compares
  // correctly (i.e. excluding NaN), this is enough.
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_float_edge =
  // Negative zero. Symta's == says 0.0 == -0.0 (IEEE behaviour);
  // the bits differ but the comparison says equal. We only verify
  // the value survives the call — not its sign bit, which we can't
  // probe without a bitcast.
  check_eq_bits 'f64 neg of 0.0'   0.0    (c_f64_neg 0.0)
  check_eq_bits 'f32 neg of 0.0'   0.0    (c_f32_neg 0.0)

  // Very small representable doubles.
  // 1e-300 is denormal-near but not denormal. Should survive.
  check_eq_bits 'f64 id 1e-300'    1e-300 (c_id_f64 1e-300)

  // Very large.
  check_eq_bits 'f64 id 1e300'     1e300  (c_id_f64 1e300)

  // The classic 0.1, which has no exact float representation.
  // Roundtrip should give the same bit pattern back.
  check_eq_bits 'f64 id 0.1'       0.1    (c_id_f64 0.1)
  check_eq_bits 'f64 id 0.2+0.1'   0.30000000000000004
                                          (c_id_f64 0.30000000000000004)

  // Magic combination: c_f64_seq y = (y*2 + 1) / 3.
  // For y=0.5: (1.0 + 1.0) / 3.0 = 0.6666...
  check_eq_bits 'f64 seq 0.5'      0.6666666666666666
                                          (c_f64_seq 0.5)
