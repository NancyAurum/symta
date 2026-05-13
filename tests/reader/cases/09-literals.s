// 09-literals.s -- collection literals.

show Src =
  R Src.parse('t')
  say "  [R]"

say "empty list:"
show '[]'

say "list literal:"
show '[1 2 3]'

say "list with mixed:"
show '[1 X "x"]'

say "splatted list (@-prefix in list ctx):"
show '[1 @Xs 2]'

say "colon-line list:"
show "Xs: 1 2 3"

say "table literal @{}:"
show '@{a!1 b!2}'

say "object literal ${}:"
show '${name!alice age!30}'

say "nested list:"
show '[[1 2] [3 4]]'

say "tuple-via-paren:"
show "(1 2 3)"
