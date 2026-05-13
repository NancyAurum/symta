// 01-numbers.s -- integer / hex / binary literal lexing.

// Dump tokens helper: for each token print "type val".
dump_tokens Src =
  Tk Src.tokenize('t')
  for Q Tk: say "  [Q.type] [Q.value]"

say "decimal:"
dump_tokens "0 1 42 999"

say "hex (0x...):"
dump_tokens "0x0 0xFF 0xDeadBeef"

say "binary (0b...):"
dump_tokens "0b0 0b1 0b1010"

say "no leading zero on radix-prefix:"
dump_tokens "2x10 16xFF"

say "digit-only identifiers fall back to int:"
dump_tokens "0 00 0123"
