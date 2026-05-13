// 07-positions.s -- row/col tracking. Critical for stack traces.

show Src =
  Tk Src.tokenize('t')
  for Q Tk: say "  [Q.value] @ r=[Q.row] c=[Q.col]"

say "single line:"
show "x y z"

say "two lines:"
show "x
y"

say "row 1 vs row 2 alignment:"
show "abc def
ghi"

say "leading whitespace on a line affects col:"
show "  spaced"

// Tabs are rejected by the tokenizer (`Unexpected no-printable
// 0x9`). That's a real constraint -- source files must use
// spaces. Worth documenting separately if we ever relax it.

say "#line N origin directive resets row:"
show "x
#line 100 \"alt\"
y"
