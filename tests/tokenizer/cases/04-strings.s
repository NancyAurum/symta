// 04-strings.s -- string-literal lexing (single + double + symbol).

dump_tokens Src =
  Tk Src.tokenize('t')
  for Q Tk: say "  [Q.type] [Q.value]"

say "single-quoted (no interpolation):"
dump_tokens "'hello'"

say "double-quoted (splice-capable, no incut):"
dump_tokens "\"hello\""

say "backtick symbol literal:"
dump_tokens "`+` `name`"

say "string with escape sequences:"
dump_tokens "'\\n \\t \\\\'"

say "splice with one incut:"
A '"a[X]b"'
dump_tokens A

say "splice with two incuts:"
B '"start[X]mid[Y]end"'
dump_tokens B

say "empty strings:"
dump_tokens "'' \"\""
