// 03-operators.s -- operator-token lexing.
// The tokenizer should split operator runs into the longest valid
// token. Disambiguation happens later in the parser.

dump_tokens Src =
  Tk Src.tokenize('t')
  for Q Tk: say "  [Q.type] [Q.value]"

say "arithmetic:"
dump_tokens "+ - * / %"

say "comparison (Symta uses >< for == and <> for !=):"
dump_tokens "< > << >> >< <>"

say "logical:"
dump_tokens "&& || ! and or"

say "bitwise (suffix-separated dashes):"
dump_tokens "-+- -^- -*- -<- ->-"

say "increment / decrement:"
dump_tokens "++ --"

say "method-call / index:"
dump_tokens ". ^ -> @"

say "assignment family:"
dump_tokens "= <= => += -= *= /= %="

say "single-quote / backtick / backslash:"
dump_tokens "\\ \\\\ '"

say "operator clusters (longest-match):"
dump_tokens "<>< >><"
