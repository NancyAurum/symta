// tc_ptr — pointer round-trip + pointer arithmetic.
//
// FFI ptr passing: Symta represents a C pointer as a fixnum-
// encoded raw 48-bit value. The runtime macro NFI_DECPTR /
// NFI_ENCPTR converts between the encoded and raw forms. If a
// bit is lost in that conversion, c_ptr_diff produces the wrong
// number.

use cffi
export tc_ptr

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_ptr =
  // Allocate a buffer through C, get it back, free it. The
  // pointer must survive Symta's encode-decode.
  P c_buf_alloc 64
  P2 c_id_ptr P
  if P >< P2
    then say "PASS ptr roundtrip via c_id_ptr"
    else say "FAIL ptr roundtrip via c_id_ptr"

  // Increment the pointer, then ask C what the difference is.
  // c_ptr_inc returns P+1; c_ptr_diff returns its first arg minus
  // its second. So (P+1) - P should be 1.
  Pi c_ptr_inc P
  check 'ptr diff = 1' 1 (c_ptr_diff Pi P)

  // Increment twice.
  Pii c_ptr_inc Pi
  check 'ptr diff = 2' 2 (c_ptr_diff Pii P)

  c_buf_free P

  // NULL ptr roundtrip via the dedicated constant. NULL on the C
  // side is 0; Symta sees the encoded form.
  Null c_const_ptr_null()
  Null2 c_id_ptr Null
  if Null >< Null2
    then say "PASS ptr NULL roundtrip"
    else say "FAIL ptr NULL roundtrip"
