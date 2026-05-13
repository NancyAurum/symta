// 08-edge.s -- empty / minimal / boundary inputs.

show Label Src =
  say "[Label]:"
  Tk Src.tokenize('t')
  say "  [Tk.n] tokens"
  for Q Tk: say "    [Q.type] [Q.value]"

show "empty"            ""
show "just space"       "   "
show "just newline"     "
"
show "single char id"   "x"
show "single digit"     "0"
show "single op"        "+"
show "trailing newline" "x
"
show "leading newline"  "
x"
