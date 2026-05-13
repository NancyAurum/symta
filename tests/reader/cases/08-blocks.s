// 08-blocks.s -- `|` block grouping at various indent depths.

show Src =
  R Src.parse('t')
  say "  [R]"

say "single | line:"
show "| a"

say "block with two stmts:"
show "| a
| b"

say "block at column 2 (inside body):"
show "f =
  | a
  | b"

say "nested blocks:"
show "f =
  | a
  | g =
    | b
    | c"

say "block as RHS:"
show "x = | a; b; c"
