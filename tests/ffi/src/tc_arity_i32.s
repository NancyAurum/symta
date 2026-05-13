// tc_arity_i32 — int arg counts from 4 (Win64 register limit) up
// past the stack-arg boundary (12 args).
//
// Win64 burns rcx/rdx/r8/r9 then spills to [rsp+32], [rsp+40], …
// If the stack-arg layout is off by one, c_sum_i32_12 will return
// a wrong sum.

use cffi
export tc_arity_i32

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_arity_i32 =
  // 4 args — all in registers on every ABI.
  check 'sum i32×4'  10  (c_sum_i32_4 1 2 3 4)
  check 'sum i32×4 alt' 0 (c_sum_i32_4 100 -50 -30 -20)

  // 6 args — fills SysV's integer pool (rdi/rsi/rdx/rcx/r8/r9)
  // exactly; on Win64 args 5 and 6 spill to stack.
  check 'sum i32×6'  21  (c_sum_i32_6 1 2 3 4 5 6)

  // 8 args — past SysV's int pool; uses 2 stack slots.
  // On Win64, args 5..8 are all on stack.
  check 'sum i32×8'  36  (c_sum_i32_8 1 2 3 4 5 6 7 8)

  // 12 args — well into the stack-arg region for both ABIs.
  // 1+2+…+12 = 78.
  check 'sum i32×12' 78  (c_sum_i32_12 1 2 3 4 5 6 7 8 9 10 11 12)

  // Same call with negative numbers — exercises sign-extension
  // of stack-slot args. Each i32 occupies a full 8-byte slot
  // on x86-64 stack; the upper 4 bytes must be sign-extended.
  // 1+2+…+12 with all negated = -78.
  check 'sum i32×12 neg' -78
        (c_sum_i32_12 -1 -2 -3 -4 -5 -6 -7 -8 -9 -10 -11 -12)
