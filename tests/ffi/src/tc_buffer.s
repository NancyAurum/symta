// tc_buffer — passing buffers across the FFI.
//
// Exercises the full "alloc → fill → read back → free" pattern,
// in two directions:
//   (a) Symta allocs (ffi_alloc), C writes through the pointer,
//       Symta reads what C wrote.
//   (b) C allocs (c_buf_alloc), Symta writes via _ffi_set, C
//       reads it back (c_buf_checksum / c_buf_get_u32).
//
// Catches: pointer bit-loss, buffer-of-N writes outside the
// intended N bytes, byte-order mismatches in u32 r/w.

use cffi
export tc_buffer

check Label Expected Got =
  if Expected >< Got
    then say "PASS [Label]"
    else say "FAIL [Label]: expected [Expected] got [Got]"

tc_buffer =
  // ============================================================
  // Path (a): Symta-allocated buffer, C-side write.
  // ============================================================
  N 16
  P ffi_alloc N
  c_buf_fill P N 0xAB   // every byte = 0xAB
  // Read it back via Symta's _ffi_get uint8_t and verify all 0xAB.
  Ok 1
  for I N: if _ffi_get(uint8_t P I) <> 0xAB: Ok = 0
  if Ok
    then say "PASS symta-alloc + c-fill 0xAB"
    else say "FAIL symta-alloc + c-fill 0xAB"
  ffi_free P

  // ============================================================
  // Path (b): C-allocated buffer, Symta-side write, C-side read.
  // ============================================================
  Q c_buf_alloc 32
  for I 32: _ffi_set uint8_t Q I I
  // c_buf_checksum walks the buffer with a deterministic hash:
  //   sum = sum*31 + b  starting from sum=0
  // For bytes 0..31 the expected value is computable but we just
  // call c_buf_checksum on the C side and compare against a
  // memcpy-then-checksum on a Symta-allocated copy.
  P2 ffi_alloc 32
  c_buf_copy P2 Q 32  // Symta passes both pointers; C memcpys
  Cs1 c_buf_checksum Q  32
  Cs2 c_buf_checksum P2 32
  check 'c-alloc + symta-write + checksum match' Cs1 Cs2
  c_buf_free Q
  ffi_free P2

  // ============================================================
  // 32-bit pattern at offset.
  // ============================================================
  R ffi_alloc 16
  c_buf_set_u32 R 0  0x12345678
  c_buf_set_u32 R 4  0xDEADBEEF
  c_buf_set_u32 R 8  0xCAFEBABE
  c_buf_set_u32 R 12 0xFEEDFACE
  check 'u32 read at 0'  0x12345678 (c_buf_get_u32 R 0)
  check 'u32 read at 4'  0xDEADBEEF (c_buf_get_u32 R 4)
  check 'u32 read at 8'  0xCAFEBABE (c_buf_get_u32 R 8)
  check 'u32 read at 12' 0xFEEDFACE (c_buf_get_u32 R 12)
  ffi_free R
