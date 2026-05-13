// 06-offside.s -- indentation-driven block parsing.

show Src =
  R Src.parse('t')
  say "  [R]"

say "multi-line if body (auto-bar between statements):"
show "f X =
  if X > 0:
    A
    B
  else C"

say "deeply nested:"
show "f =
  if X:
    if Y:
      A
    else B
  else C"

say "for-loop body:"
show "g Xs =
  for X Xs:
    say X
    process X"

say "when body, multi-statement:"
show "h =
  when X:
    A
    B"
