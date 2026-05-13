// 04-control.s -- if/then/else, when, less, case-style if:.

show Src =
  R Src.parse('t')
  say "  [R]"

say "inline if/then/else:"
show "if X then A else B"

say "inline if with `:`:"
show "if X: A else B"
show "if X: A"

say "if: cond-chain form (each clause is `Cnd = Body`):"
show "if: A = 1; B = 2; 1 = 0"

say "when:"
show "when X: A"

say "less (= unless):"
show "less X: A"

say "elif form rewriting:"
show "if X then A elif Y then B else C"
