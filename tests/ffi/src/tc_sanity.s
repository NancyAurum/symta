// tc_sanity — load + call the simplest possible FFI fn.
//
// If this fails, the dynamic-loader path is broken or the FFI
// dispatcher is wholly non-functional. Every other case depends
// on this one passing.

use cffi
export tc_sanity

tc_sanity =
  V cffi_version()
  if V >< "cffilib-1.0":
    say "PASS sanity: cffi_version = [V]"
  else
    say "FAIL sanity: cffi_version returned [V]"
