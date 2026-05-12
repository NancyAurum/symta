// 05-loops.s -- iteration constructs
//
// Run:  symta -f examples/05-loops.s

// `times` runs Body N times with Var bound to 0..N-1.
times I 5: say "times: [I]"

// `for` iterates over a list.
Xs: red green blue
for X Xs: say "for: [X]"

// Multi-statement body via indentation.
for X Xs:
  Upper X.title
  say "  [X] -> [Upper]"

// `map` returns a list of results, one per iteration.
Squares map I 5: I*I
say "first 5 squares: [Squares]"

// `dup` is a quick generator. `dup I 5: I*I` is the same as
// `map I 5: I*I`, but is implemented as inline list construction.
say "dup:    [dup I 5 I*I]"

// While loop: needs a declared variable to mutate.
I 0
while I < 5:
  say "while: [I]"
  I+               // post-increment

// `pass` skips to the next iteration; `done` breaks the loop.
K 0
while K < 7:
  K+
  when K >< 4: pass     // skip 4
  when K >< 6: done     // stop at 6
  say "skip/break: [K]"
