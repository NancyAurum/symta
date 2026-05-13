// tc_ptr — pointer round-trip + pointer arithmetic.
//
// FFI ptr passing: Symta represents a raw C pointer as a numeric
// `ptr` value. We don't compare ptrs with `><` here — Symta's
// equality on pointer-typed values tries to dereference them in
// some paths, which is not part of the FFI contract. Instead we
// verify the round-trip via C-side operations: ask C to compute
// the difference between the pointer it gave us and the same
// pointer we passed back, and assert it's 0.

use cffi
export tc_ptr

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_ptr =
  // Allocate a buffer through C, then ask C to verify the round-
  // trip by computing the difference between two copies of the
  // pointer (one that went through c_id_ptr).
  P c_buf_alloc 64
  P2 c_id_ptr P
  check 'ptr roundtrip via c_id_ptr' 0 (c_ptr_diff P P2)

  // Increment the pointer, then ask C what the difference is.
  // c_ptr_inc returns P+1; c_ptr_diff returns its first arg minus
  // its second. So (P+1) - P should be 1.
  Pi c_ptr_inc P
  check 'ptr diff = 1' 1 (c_ptr_diff Pi P)

  // Increment twice.
  Pii c_ptr_inc Pi
  check 'ptr diff = 2' 2 (c_ptr_diff Pii P)

  c_buf_free P

  // NULL ptr round-trip via the dedicated constant. We round-trip
  // through c_id_ptr and again verify via c_ptr_diff (NULL - NULL
  // = 0).
  Null c_const_ptr_null()
  Null2 c_id_ptr Null
  check 'ptr NULL roundtrip' 0 (c_ptr_diff Null Null2)
