// case.s -- exercises the `case` pattern-match macro.
//
// Run with:  symta -f tests-macros/cases/case.s

// --- 1. Plain literal match ---
classify_n N = case N:
  0       = "zero"
  1       = "one"
  2       = "two"
  Else    = "many"

say: classify_n 0
say: classify_n 1
say: classify_n 2
say: classify_n 7

// --- 2. Alternation with + ---
day_kind D = case D:
  \mon+\tue+\wed+\thu+\fri = "weekday"
  \sat+\sun                 = "weekend"
  Else                       = "?"

say: day_kind \mon
say: day_kind \sat
say: day_kind \xx

// --- 3. Type-predicate guard `X<int?` ---
typeof V = case V:
  X<int?  = "int [X]"
  X<text? = "text [X]"
  X<list? = "list of [X.n]"
  Else    = "other"

say: typeof 42
say: typeof "hello"
say: typeof [1 2 3]
say: typeof No

// --- 4. List destructuring patterns ---
list_shape Xs = case Xs:
  []       = "empty"
  [A]      = "single [A]"
  [A B]    = "pair [A] [B]"
  [A B C]  = "triple [A] [B] [C]"
  [A @Bs]  = "head [A] tail of [Bs.n]"

say: list_shape []
say: list_shape [99]
say: list_shape [1 2]
say: list_shape [1 2 3]
say: list_shape [1 2 3 4 5]

// --- 5. Multi-clause via `|` separator (no `:` after case) ---
palindrome Xs = case Xs.l
  [S @Xs $S] | palindrome Xs
  []+[X]     | 1

say: palindrome "racecar"
say: palindrome "noon"
say: palindrome "hello"

// --- 6. Else captures the whole value ---
describe V = case V:
  0    = "zero"
  Else = "got [Else]"

say: describe 0
say: describe 42
say: describe "x"
