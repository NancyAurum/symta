// Headless gfx-FFI golden-image regression suite.
//
// Builds the cases registered in cases.s, snapshots each as a PNG
// into out/, and compares against the matching golden in ../golden/.
// Pure Symta + ffi/gfx.ffi; no SDL, no window — runs on any
// build host. See ../README.md.

use gfx harness cases

run_all_cases
