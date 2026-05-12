// 01-arithmetic.s -- variables and arithmetic
//
// Variable names start with an Uppercase letter; function names
// start with anything else. This split makes the syntax less noisy
// since arguments don't need parens.
//
// Run:  symta -f examples/01-arithmetic.s

A 123        // declare A
B 456        // declare B

say "addition:        [A+B]"
say "subtraction:     [B-A]"
say "multiplication:  [A*B]"
say "integer div:     [B/A]"
say "remainder:       [B%A]"
say "exponentiation:  [A^^3]"
say "average:         [(A+B)/2]"

// `=` reassigns an already-declared variable.
A = 789
say "now A = [A]"

// C-like compound assignment is supported.
B *= 3
say "now B = [B]"

// Floating-point numbers. Symta is strongly typed: an int does not
// silently coerce to a float, so call `.float` when mixing them.
PI 3.14159265
say "2 * PI ~ [2.0*PI]"
say "PI * 5^2 ~ [PI*5.float*5.float]"

// Comparison operators return 1 or 0:
//   ><  equal       <>  not-equal
//   <   less        >   greater
//   <<  less-eq     >>  greater-eq
say "5 >< 5 = [5><5]"
say "5 <  6 = [5<6]"
say "5 >> 5 = [5>>5]"

// `not` flips truthiness; 0 is false, anything else is true.
say "not 0 = [not 0]"
say "not 7 = [not 7]"
