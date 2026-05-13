// Top-level entry for the adaptive map regression suite.
//
// Each test case is a module that exports `tc_<name>`. When imported
// it just defines the function; the dispatcher below picks one by
// --case=NAME and runs it. The case prints PASS / FAIL lines; the
// shell driver in ../run.sh asserts no FAIL is present and that
// every line matches the captured golden.

use cls
    tc_empty tc_bitmap0 tc_bitmap1 tc_int tc_text tc_generic
    tc_promote tc_gid tc_mixed tc_iteration tc_stress

RunCase \empty
for A main_args():
  when A.is_text:
    if A.begin '--case=': RunCase = "[A.drop 7]"

case RunCase:
  'empty'     = tc_empty
  'bitmap0'   = tc_bitmap0
  'bitmap1'   = tc_bitmap1
  'int'       = tc_int
  'text'      = tc_text
  'generic'   = tc_generic
  'promote'   = tc_promote
  'gid'       = tc_gid
  'mixed'     = tc_mixed
  'iteration' = tc_iteration
  'stress'    = tc_stress
  Else        = say "no such case: [RunCase]"
