// tc_arity_f64 — double arg counts past the float-register
// pool (SysV: 8 xmm regs).
//
// On Win64, only xmm0..3 are available for floats by position;
// args 5+ spill to stack. On SysV, args 1..8 fit in xmm0..7,
// args 9+ spill. c_sum_f64_10 exercises both spill paths.
//
// Float stack-arg passing on x86-64 is more sensitive than int:
// the slot is 8 bytes but the value is a double, which means
// the entire slot is meaningful. Any "high 32 cleared" sloppy
// codegen breaks here.

use cffi
export tc_arity_f64

check_near Label Expected Got =
  Diff Expected - Got
  if Diff.abs.float < 0.000001  // 1e-6; loose tolerance avoids compiler %.8f precision loss -- see TODO READER-1
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_arity_f64 =
  // 4 doubles — register-only on every ABI.
  check_near 'sum f64×4'   10.0   (c_sum_f64_4 1.0 2.0 3.0 4.0)
  check_near 'sum f64×4 alt' -3.5
        (c_sum_f64_4 -1.5 -2.0 -3.0 3.0)

  // 6 doubles — register-only on SysV (xmm0..5), Win64 fills
  // xmm0..3 + 2 stack slots.
  check_near 'sum f64×6'   21.0
        (c_sum_f64_6 1.0 2.0 3.0 4.0 5.0 6.0)

  // 8 doubles — fills SysV's xmm pool exactly. Win64 has 4 on
  // stack.
  check_near 'sum f64×8'   36.0
        (c_sum_f64_8 1.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0)

  // 10 doubles — past SysV's xmm pool. The last two doubles
  // are stack args on every ABI.
  // 1+2+…+10 = 55
  check_near 'sum f64×10'  55.0
        (c_sum_f64_10 1.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 10.0)

  // 10 negative doubles — checks high-half preservation on
  // stack slots.
  check_near 'sum f64×10 neg' -55.0
        (c_sum_f64_10 -1.0 -2.0 -3.0 -4.0 -5.0
                      -6.0 -7.0 -8.0 -9.0 -10.0)
