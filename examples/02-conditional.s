// 02-conditional.s -- conditional evaluation
//
// Run:  symta -f examples/02-conditional.s

// The classic `if/then/else`. Any non-zero value is true.
say if 1: "yes" else "no"
say if 0: "yes" else "no"

// `when C: Body`  runs Body only when C is true.
// `less C: Body`  runs Body only when C is false (the inverse).
X 7
when X > 5: say "X is greater than 5"
less X > 100: say "X is at most 100"

// Multi-branch `if`: a chain of `Cond = Body` lines, ending in
// `1 = ...` for the catch-all.
classify N =
  if: N >< 0  = "zero"
      N <  0  = "negative"
      N <  10 = "small"
      N <  100 = "medium"
      1       = "large"

say: classify 0
say: classify -3
say: classify 5
say: classify 42
say: classify 9000

// `and` / `or` chain conditions. Note the colon; it forces the
// expression to be parsed as a single boolean argument to `when`.
A 1
B 0
when A and not B: say "A is on and B is off"
when A or B: say "at least one is on"
