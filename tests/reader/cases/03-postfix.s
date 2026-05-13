// 03-postfix.s -- postfix calls: .field, ^method, [idx], (args).

show Src =
  R Src.parse('t')
  say "  [R]"

say "method/field via dot:"
show "X.f"
show "X.f.g"
show "X.1"

say "method via caret (passes through Symta's value lookup):"
show "X^f"

say "arrow:"
show "X->f"

say "function call:"
show "f(X)"
show "f(X Y)"
show "f(X Y Z)"

say "chained call + method:"
show "f(X).g"
show "X.f(Y)"

say "bracket index:"
show 'Xs[0]'
show 'Xs[Y].f'

say "curly map:"
show 'Xs{?*2}'

say "list-literal vs method-call disambiguation:"
show 'Xs [1 2]'
show "Xs.tail"
