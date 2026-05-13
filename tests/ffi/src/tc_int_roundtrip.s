// tc_int_roundtrip — i32 identity + constant returns.
//
// The single best smoke test for "did the bits go in and come
// out unchanged?" If int args are getting truncated (e.g. 32 ->
// 8 on the wrong register), or if the return is being sign-
// extended weirdly, these catch it.

use cffi
export tc_int_roundtrip

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_int_roundtrip =
  // Identity on a handful of common values.
  check 'i32 id 0'      0    (c_id_i32 0)
  check 'i32 id 1'      1    (c_id_i32 1)
  check 'i32 id -1'     -1   (c_id_i32 -1)
  check 'i32 id 42'     42   (c_id_i32 42)
  check 'i32 id -42'    -42  (c_id_i32 -42)
  check 'i32 id 65536'  65536 (c_id_i32 65536)
  check 'i32 id 1<<30'  1073741824 (c_id_i32 1073741824)

  // Constants that exercise the boundary values.
  check 'i32_max const'  2147483647   (c_const_i32_max())
  check 'i32_min const'  -2147483648  (c_const_i32_min())
  check 'i32_-1 const'   -1           (c_const_i32_neg1())
