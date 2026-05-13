// tc_interleave — int/float mixed arguments.
//
// The single most important test for catching SysV vs Win64
// bugs in a backend. The two ABIs disagree about what "argument
// position 1" means when arg 0 is an int and arg 1 is a float:
//
//   - Win64 says: each POSITION takes its register slot. Int
//     at position 0 → rcx. Float at position 1 → xmm1 (not
//     xmm0!). So (int, float) consumes rcx + xmm1.
//
//   - SysV says: separate POOLS. Int at position 0 → rdi (int
//     pool slot 0). Float at position 1 → xmm0 (float pool
//     slot 0). So (int, float) consumes rdi + xmm0.
//
// If a backend uses one convention's semantics on the other
// platform, these tests catch it immediately.

use cffi
export tc_interleave

check_near Label Expected Got =
  Diff Expected - Got
  if Diff.abs.float < 1e-9
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_interleave =
  // 4 args, alternating starting with int.
  // 1 + 2.0 + 3.0 + 4 = 10.0
  check_near 'mix iffi'  10.0  (c_mix4_iffi 1 2.0 3.0 4)

  // 4 args, alternating starting with float.
  // 1.0 + 2 + 3 + 4.0 = 10.0
  check_near 'mix fiif'  10.0  (c_mix4_fiif 1.0 2 3 4.0)

  // 4 args, floats first then ints.
  check_near 'mix ffii'  10.0  (c_mix4_ffii 1.0 2.0 3 4)

  // 4 args, ints first then floats.
  check_near 'mix iiff'  10.0  (c_mix4_iiff 1 2 3.0 4.0)

  // 6 args, irregular pattern (i f i i f i).
  // 1 + 2.0 + 3 + 4 + 5.0 + 6 = 21.0
  check_near 'mix ifiifi' 21.0
        (c_mix6_ifiifi 1 2.0 3 4 5.0 6)

  // 6 args, (f i i f f i).
  // 1.0 + 2 + 3 + 4.0 + 5.0 + 6 = 21.0
  check_near 'mix fiiffi' 21.0
        (c_mix6_fiiffi 1.0 2 3 4.0 5.0 6)

  // Distinct magnitudes catch "did the slot-N int land where
  // slot-N float was expected" — if the buckets are swapped
  // the sum is wildly wrong, not subtly so.
  check_near 'mix iffi distinct' 1234.5
        (c_mix4_iffi 1000 200.5 30.0 4)
  // 1000 + 200.5 + 30.0 + 4 = 1234.5
