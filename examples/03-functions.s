// 03-functions.s -- functions, recursion, lambdas
//
// Run:  symta -f examples/03-functions.s

// Plain function: arguments, then `=`, then body.
greet Name = say "Hello, [Name]!"

greet "World"
greet "Symta"
greet("Reader")           // parens form is also fine

// Recursion: classic factorial.
factorial N = if N >< 0 then 1 else N * factorial(N-1)

say "5!  = [factorial 5]"
say "10! = [factorial 10]"

// Keyword argument with default. `Name!"World"` declares a keyword
// parameter. Callers pass it as `hi Name!"value"`.
hi Name!"World" = say "Hi, [Name]!"

hi
hi Name!"there"
hi Name!"keyword caller"

// Anonymous function bound to a "verb" with `&name | X => Body`.
&square | X => X * X
say: square 9

// Two-arg lambda.
&dist | A B => (A*A + B*B).float.sqrt
say: dist 3 4

// `^` -- Forth-style "apply on the left":
//   "World" ^ say        is the same as   say "World"
//   3 ^ square           is the same as   square 3
"World" ^ say
say: 3 ^ square

// Local helper inside a function: same `=` syntax, scoped to body.
hypotenuse A B =
  sq X = X * X
  (sq(A) + sq(B)).float.sqrt

say: hypotenuse 5 12

// Multi-line body with indentation.
verbose X =
  say "got [X]"
  X * 2
say: verbose 21
