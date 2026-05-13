// 07-splice.s -- string-splice interpolation parsing.

show Src =
  R Src.parse('t')
  say "  [R]"

say "plain splice, no incut:"
show '"hello"'

say "single-incut:"
show '"x=[X]"'

say "incut + suffix:"
show '"x=[X], done"'

say "two incuts:"
show '"a=[A], b=[B]"'

say "incut with expression:"
show '"sum=[X+Y]"'

say "method call inside incut:"
show '"sz=[Xs.n]"'
