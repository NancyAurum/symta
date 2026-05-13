// tc_stress — high-volume calls to catch per-call state leakage.
//
// The classic FFI bug class: a register-pool consumed-counter
// isn't reset between calls, or a stack-arg buffer is reused
// without clearing, or the trampoline writes a return value to
// a stale slot. These bugs typically affect the SECOND call
// onward — the first call lands on freshly-zeroed memory and
// looks fine.
//
// Strategy: call c_hash_i32 in a tight loop with sequential
// inputs and verify every return value matches the C-side hash
// formula. If even one call returns a stale value from a previous
// invocation's args, the chain breaks.

use cffi
export tc_stress

// Reimplement c_hash_i32 in Symta so we have a Symta-side
// reference to compare against. Same formula:
//   v = (uint32)x
//   v ^= v >> 16
//   v *= 0x7feb352d
//   v ^= v >> 15
//   v *= 0x846ca68b
//   v ^= v >> 16
// All ops are uint32; Symta uses -*-, -+-, -^- for bitwise.
hash_ref X =
  V X -*- 0xFFFFFFFF
  V = V -^- (V ->- 16)
  V = (V * 0x7feb352d) -*- 0xFFFFFFFF
  V = V -^- (V ->- 15)
  V = (V * 0x846ca68b) -*- 0xFFFFFFFF
  V = V -^- (V ->- 16)
  V

tc_stress =
  // 1000 sequential calls.
  Fail 0
  for I 1000:
    Expected hash_ref I
    Got c_hash_i32 I
    if Expected <> Got: Fail+
  if Fail >< 0
    then say "PASS 1000 sequential c_hash_i32 calls"
    else say "FAIL [Fail] of 1000 c_hash_i32 calls mismatched"

  // Same 1000 calls but with negative inputs — exercises the
  // sign-extension path for the int → u32 reinterpretation.
  Fail2 0
  for I 1000:
    X 0 - I
    Expected hash_ref X
    Got c_hash_i32 X
    if Expected <> Got: Fail2+
  if Fail2 >< 0
    then say "PASS 1000 c_hash_i32 calls with negative input"
    else say "FAIL [Fail2] of 1000 negative-input calls mismatched"

  // Interleaved arity: switch between c_sum_i32_4 and
  // c_sum_f64_4 calls in a loop. A backend that leaks "last
  // call's float reg state" into "this call's int call" breaks
  // here.
  Fail3 0
  for I 100:
    S c_sum_i32_4 I I+1 I+2 I+3
    if S <> 4*I + 6: Fail3+
    D c_sum_f64_4 (I.float) ((I+1).float) ((I+2).float) ((I+3).float)
    if D <> (4*I + 6).float: Fail3+
  if Fail3 >< 0
    then say "PASS 200 interleaved int/float-sum calls"
    else say "FAIL [Fail3] of 200 interleaved calls mismatched"
