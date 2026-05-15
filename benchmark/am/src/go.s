// Adaptive-map benchmark suite dispatcher.
//
// Each benchmark module exports `bn_<name>` and prints
// timing data in a parseable plaintext format that run.sh
// scrapes into a comparison table.

use cls
    bn_int bn_text bn_generic bn_bitmap bn_promote bn_iter bn_adv

RunCase \all
for A main_args():
  when A.is_text:
    if A.begin '--case=': RunCase = "[A.drop 7]"

case RunCase:
  'int'     = bn_int
  'text'    = bn_text
  'generic' = bn_generic
  'bitmap'  = bn_bitmap
  'promote' = bn_promote
  'iter'    = bn_iter
  'adv'     = bn_adv
  'all' =
    bn_int
    bn_text
    bn_generic
    bn_bitmap
    bn_promote
    bn_iter
    bn_adv
  Else = say "no such case: [RunCase]"
