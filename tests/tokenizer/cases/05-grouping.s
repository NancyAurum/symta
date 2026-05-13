// 05-grouping.s -- bracket/paren/curly grouping (top-level token
// only -- inner contents come back as a sub-list on .value).

show Src =
  Tk Src.tokenize('t')
  for Q Tk: say "  type=[Q.type] val=[Q.value]"

say "parens:"
show '(1 2 3)'

say "brackets:"
show '[a b c]'

say "curly:"
show '{x y z}'

say "table form @{}:"
show '@{a!1 b!2}'

say "object form ${}:"
show '${name!alice}'

say "nested:"
show '(a [b {c} d])'

say "mixed with op outside:"
show 'f(x)+g[y]'
