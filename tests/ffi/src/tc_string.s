// tc_string — text → const char* round-trip and inspection.
//
// Symta marshals text args via api.text_chars which returns a
// pointer to the null-terminated UTF-8 cstring. The pointer
// survives until the next GC; for FFI calls that consume the
// string synchronously this is always safe.

use cffi
export tc_string

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_string =
  check 'strlen empty'    0   (c_strlen "")
  check 'strlen one'      1   (c_strlen "x")
  check 'strlen 5'        5   (c_strlen "hello")
  check 'strlen 13'       13  (c_strlen "Hello, World!")

  // Byte-sum gives us a hash. "ABC" = 65+66+67 = 198.
  check 'bytesum ABC'     198 (c_str_bytesum "ABC")
  check 'bytesum 0'       0   (c_str_bytesum "")

  // strcmp returns 0 for equality, non-zero otherwise. We don't
  // check the exact sign — the contract is only `(== 0)`.
  if (c_strcmp "abc" "abc") >< 0
    then say "PASS strcmp abc==abc"
    else say "FAIL strcmp abc==abc"

  if (c_strcmp "abc" "abd") <> 0
    then say "PASS strcmp abc<>abd"
    else say "FAIL strcmp abc<>abd"

  // First-occurrence offset.
  check 'strchr ofs of `,` in Hello,World' 5
        (c_strchr_ofs "Hello,World" ','.code)
  check 'strchr no match' -1
        (c_strchr_ofs "no comma here" ','.code)
