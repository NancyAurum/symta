// tc_uint32 — u32 round-trip + boundary values.
//
// Symta passes u32 by zero-extending into a 64-bit slot. The C
// side reads `uint32_t`. The FFI return path packs the high 32
// bits as zeros.

use cffi
export tc_uint32

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_uint32 =
  check 'u32 id 0'         0           (c_id_u32 0)
  check 'u32 id 1'         1           (c_id_u32 1)
  check 'u32 id 0xFF'      0xFF        (c_id_u32 0xFF)
  check 'u32 id 0xFFFF'    0xFFFF      (c_id_u32 0xFFFF)
  check 'u32 id 0x10000'   0x10000     (c_id_u32 0x10000)
  check 'u32 id 0x7FFFFFFF' 0x7FFFFFFF (c_id_u32 0x7FFFFFFF)

  // Max u32. Symta i32 can't hold this so we use the hex literal.
  check 'u32_max const'    0xFFFFFFFF  (c_const_u32_max())
