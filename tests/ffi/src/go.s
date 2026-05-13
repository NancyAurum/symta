// Top-level entry for the FFI regression suite.
//
// Each test case is a module that, when imported, runs assertions
// at top level and prints PASS / FAIL lines. The shell driver
// at ../run.sh dispatches one case at a time via --case=NAME.
//
// Adding a new case: write a test fn in src/tc_<name>.s that
// exports `tc_<name>` (no args, prints PASS/FAIL), then append it
// to the `use` line and the `case` dispatch below.

use cls cffi
    tc_sanity tc_int_roundtrip tc_uint32 tc_float tc_double
    tc_ptr tc_string tc_arith tc_arity_i32 tc_arity_f64
    tc_interleave tc_buffer tc_str_ops tc_float_edge tc_stress
    tc_libc tc_zlib

RunCase \sanity
for A main_args():
  when A.is_text:
    if A.begin '--case=': RunCase = "[A.drop 7]"

case RunCase:
  'sanity'         = tc_sanity
  'int_roundtrip'  = tc_int_roundtrip
  'uint32'         = tc_uint32
  'float'          = tc_float
  'double'         = tc_double
  'ptr'            = tc_ptr
  'string'         = tc_string
  'arith'          = tc_arith
  'arity_i32'      = tc_arity_i32
  'arity_f64'      = tc_arity_f64
  'interleave'     = tc_interleave
  'buffer'         = tc_buffer
  'str_ops'        = tc_str_ops
  'float_edge'     = tc_float_edge
  'stress'         = tc_stress
  'libc'           = tc_libc
  'zlib'           = tc_zlib
  Else             = say "no such case: [RunCase]"
