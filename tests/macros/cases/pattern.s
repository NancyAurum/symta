// pattern.s -- exercises the case/{} pattern-matcher operators.
//
// The pattern grammar is documented in sbe.md's "Pattern Matching"
// chapter (see also `expand_hole` in symta/src/macro.s).
//
// Run with:  symta -f tests-macros/cases/pattern.s


// ----------------------------------------------------------------
// 1. `_` -- matches anything, doesn't bind. `Else` -- the default.
// ----------------------------------------------------------------
classify Xs = case Xs:
  [_ _ _] = "exactly three"
  Else    = "other"

say "wildcards:"
say: classify [1 2 3]
say: classify [1 2]


// ----------------------------------------------------------------
// 2. `$Expr` -- literal-match the result of evaluating Expr.
//    Used to compare with previously bound variables.
// ----------------------------------------------------------------
X 5
check V = case V:
  $X   = "matched 5"
  Else = "no"

say "literal-match $-Expr:"
say: check 5
say: check 7


// ----------------------------------------------------------------
// 3. `-Var` -- match non-zero, bind to Var.
// ----------------------------------------------------------------
nonzero V = case V:
  -Y   = "got [Y]"
  Else = "zero"

say "non-zero binding:"
say: nonzero 0
say: nonzero 99


// ----------------------------------------------------------------
// 4. `A+B` -- alternation. `A<B` -- bind A=B + match A.
//    Note: in patterns, `+` is OR, not addition.
// ----------------------------------------------------------------
day_kind D = case D:
  \mon+\tue+\wed+\thu+\fri = "weekday"
  \sat+\sun                = "weekend"
  Else                      = "?"

say "alternation:"
say: day_kind \mon
say: day_kind \sat
say: day_kind \xx


// ----------------------------------------------------------------
// 5. `X<type?` -- type-predicate guard, bind X to the matched value.
// ----------------------------------------------------------------
typeof V = case V:
  X<int?  = "int [X]"
  X<text? = "text [X]"
  X<list? = "list of [X.n]"
  Else    = "other"

say "type guard:"
say: typeof 42
say: typeof "hello"
say: typeof [1 2 3]
say: typeof No


// ----------------------------------------------------------------
// 6. `[A @As]` -- head/tail destructure. `[@Bs B]` -- lead/last.
//    Also as function-argument patterns.
// ----------------------------------------------------------------
// Avoid `length` -- it may collide with type-specific methods.
mylen Xs = case Xs:
  [] = 0
  [_ @T] = mylen(T) + 1

say "fn-arg destructure:"
say: mylen [10 20 30 40]


// ----------------------------------------------------------------
// 7. Split-on-pivot: `[@Ys pivot @Zs]`.
// ----------------------------------------------------------------
say "split on pivot:"
case "we have a needle in the middle".split^' ':
  [@Ys needle @Zs] = say "  before=[Ys] after=[Zs]"
  Else = say "  no needle"


// ----------------------------------------------------------------
// 8. `&Var` setter inside a pattern -- modify in place.
// ----------------------------------------------------------------
Xs: a b c d
say "before: Xs=[Xs]"
case Xs [X@&Xs]:
  say "popped [X], Xs=[Xs]"


// ----------------------------------------------------------------
// 9. Inline `:` lambda form: `O(default: pattern = body; ...)`.
// ----------------------------------------------------------------
say "inline pattern lambda:"
say: [1 2 3]([X Y Z] = "[X]-[Y]-[Z]")
say: [1 2]([X Y Z] = "[X]-[Y]-[Z]" ; Else = "no triple")


// ----------------------------------------------------------------
// 10. `case` with body via `|` (multi-statement clause).
// ----------------------------------------------------------------
say "multi-stmt clause:"
go V = case V:
  X<int? =
    say "got int [X]"
    say "(double=[X*2])"
  Else =
    say "got something else"

go 7
go "x"
