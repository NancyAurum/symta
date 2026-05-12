// 12-macro -- defining and using macros
//
// A macro is a function that runs at *compile time*. Its arguments
// arrive unevaluated (as AST), and it returns new AST. Most of
// Symta's "keywords" -- when, while, for, case, ... -- are macros
// you could have written yourself.
//
// To declare a macro at module scope, put it in its own .s file
// (here `mymac.s`) and `export 'name'` it. Then `use mymac` in
// the consuming module.
//
// Run:  symta examples/12-macro && examples/12-macro/go.exe

use mymac

// `pi 2.0` is replaced at compile time with `6.28318530`.
say "pi(2.0)  = [pi 2.0]"
say "pi(0.5)  = [pi 0.5]"

// `unless C: Body` -- our own version of `less`.
X 5
unless X >< 0: say "X is non-zero (X = [X])"

// `swap` swaps two variables in place. Note we pass them as raw
// AST -- a function couldn't do that, since it'd see only their
// values.
A 11
B 22
say "before: A=[A] B=[B]"
swap A B
say "after:  A=[A] B=[B]"

// `repeat N Body` -- a compile-time-unrolled loop.
repeat 3:
  say tick

say done
