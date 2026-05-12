// 06-pattern.s -- pattern matching, the heart of Symta
//
// Run:  symta -f examples/06-pattern.s

// Destructuring: bind several variables from a list at once.
Xs: 10 20 30
[X Y Z] Xs
say "X=[X]  Y=[Y]  Z=[Z]"

// `@As` matches the rest of the list.
[A @As] Xs
say "head=[A]  tail=[As]"

// `@` can be at the end too: capture all-but-last.
[@Bs B] Xs
say "lead=[Bs]  last=[B]"

// `case Expr` branches by pattern. The first match wins.
classify Val = case Val:
  0       = "zero"
  1+2+3   = "one to three"
  X<int?  = "some int [X]"
  Else    = "other: [Else]"

say: classify 0
say: classify 2
say: classify 99
say: classify "abc"

// Patterns can be used as function argument lists. A 2-element
// list will be destructured into X and Y.
v_size [X Y] = (X*X+Y*Y).float.sqrt
say: v_size [3 4]
say: v_size [5 12]

// Recursion + pattern matching: classic palindrome check.
palindrome Xs = case Xs.l
  [S @Xs $S] | palindrome Xs        // strip equal head and tail
  []+[X]     | 1                    // empty or single element
                                    // (no Else => 0 by default)

say: palindrome "racecar"
say: palindrome "noon"
say: palindrome "hello"

// Quicksort -- one line of pattern matching.
qsort@r H,@T = @T{:?<H}^r,H,@T{?<H=}^r
say: qsort [3 1 4 1 5 9 2 6 5 3 5]
