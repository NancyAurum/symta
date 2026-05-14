// Runtime / interpreter benchmark suite dispatcher.
//
// Each benchmark module exports `bn_<name>` and prints
// timing data in a parseable plaintext format that run.sh
// scrapes into a comparison table.
//
// The hot loops here are designed so that the dispatch path
// dominates the measurement. Each kernel exercises a different
// opcode mix (arithmetic, branches, function calls, method
// calls, list ops, raw loop overhead) so that a change to the
// dispatch loop (e.g. RT-1's computed-goto rewrite) shows up
// where it matters most.

use cls
    bn_loop bn_arith bn_branch bn_call bn_mcall bn_list bn_gc

RunCase \all
for A main_args():
  when A.is_text:
    if A.begin '--case=': RunCase = "[A.drop 7]"

case RunCase:
  'loop'   = bn_loop
  'arith'  = bn_arith
  'branch' = bn_branch
  'call'   = bn_call
  'mcall'  = bn_mcall
  'list'   = bn_list
  'gc'     = bn_gc
  'all' =
    bn_loop
    bn_arith
    bn_branch
    bn_call
    bn_mcall
    bn_list
    bn_gc
  Else = say "no such case: [RunCase]"
