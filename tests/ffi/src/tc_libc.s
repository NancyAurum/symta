// tc_libc — call into the C standard library / msvcrt / libc.so.
//
// "Available on every platform" — even tiny embedded ones — so
// this is the canonical "FFI to a real shared library" test that
// will be runnable on every port target (Linux, macOS, Win95+,
// RISC OS, AArch64).
//
// We deliberately use very simple libc functions whose behaviour
// is unambiguous and stable across libc versions. If a port can't
// pass these, it can't call ANY real library.

use cls
export tc_libc

// Resolve a symbol from libc. The library name varies per
// platform:
//   - Windows:    "msvcrt"      (or "ucrtbase" on newer)
//   - macOS:      "libSystem"   (loaded automatically by dyld)
//   - Linux:      "libc.so.6"   (or just "c")
//   - RISC OS:    "SharedCLibrary"
//
// On Windows + macOS + Linux, ffi_load can typically resolve
// "msvcrt" / "c" / "libSystem" without a path. We try a small
// list of candidate names and use the first one that works.

libc_resolve Name =
  for Lib [msvcrt c System libc]:
    F ffi_load Lib Name
    when F: ret F
  No

tc_libc =
  // strlen(const char*) -> size_t (treated as i32 — fits for
  // any test string we use here).
  &StrlenFn libc_resolve \strlen
  if no StrlenFn:
    say "SKIP libc strlen (no libc resolvable)"
    ret

  R1 _ffi_call \(int ptr) StrlenFn "hello"
  if R1 >< 5
    then say "PASS libc strlen('hello') = 5"
    else say "FAIL libc strlen('hello') = [R1] (expected 5)"

  R2 _ffi_call \(int ptr) StrlenFn ""
  if R2 >< 0
    then say "PASS libc strlen('') = 0"
    else say "FAIL libc strlen('') = [R2] (expected 0)"

  // memset(void*, int, size_t) -> void*: write a byte pattern.
  &MemsetFn libc_resolve \memset
  if no MemsetFn:
    say "SKIP libc memset (not resolved)"
    ret

  N 32
  P ffi_alloc N
  // memset returns the buffer; ignore return.
  _ffi_call \(ptr ptr int int) MemsetFn P 0x42 N
  Ok 1
  for I N: if _ffi_get(uint8_t P I) <> 0x42: Ok = 0
  if Ok
    then say "PASS libc memset 0x42×32"
    else say "FAIL libc memset 0x42×32"
  ffi_free P

  // memcpy: round-trip a small buffer.
  &MemcpyFn libc_resolve \memcpy
  if no MemcpyFn:
    say "SKIP libc memcpy (not resolved)"
    ret

  Src ffi_alloc 8
  Dst ffi_alloc 8
  for I 8: _ffi_set uint8_t Src I I*7+1
  _ffi_call \(ptr ptr ptr int) MemcpyFn Dst Src 8
  Ok2 1
  for I 8:
    A _ffi_get uint8_t Src I
    B _ffi_get uint8_t Dst I
    if A <> B: Ok2 = 0
  if Ok2
    then say "PASS libc memcpy 8 bytes round-trip"
    else say "FAIL libc memcpy 8 bytes round-trip"
  ffi_free Src
  ffi_free Dst
