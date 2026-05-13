// 02-symbols.s -- identifier / symbol lexing rules.

dump_tokens Src =
  Tk Src.tokenize('t')
  for Q Tk: say "  [Q.type] [Q.value]"

say "plain lowercase:"
dump_tokens "foo bar baz"

say "uppercase (still symbol type, semantics differ later):"
dump_tokens "Foo BAR Baz"

say "underscore allowed:"
dump_tokens "_x x_ x_y _"

say "digits in tail:"
dump_tokens "x1 abc42 x_3"

say "question / tilde in identifier (Symta convention):"
dump_tokens "is_int? ~x x~y"

say "language keywords are tokenized as their own type:"
dump_tokens "if then else elif and or"
