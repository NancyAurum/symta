// parser-compare.s -- side-by-side AST diff: Symta parse_tokens vs
// the C reader_parse_tokens (exposed as parse_tokens_c_).
//
// For each test input, parse via both paths and compare the
// printed AST. Report first divergence with context.
//
// Run:  symta -f tests-runtime/parser-compare.s

dump X =
  // Round-trip through "[X]" to get a canonical string form.
  R "[X]"
  R

probe Label Src =
  // Symta side: text.parse (parse_tokens + parse_strip).
  S_full Src.parse('t')
  // C side: parse_tokens_c_ + parse_strip_c_.
  Tk Src.tokenize('t')
  Bars add_bars_c_ Tk
  C_raw parse_tokens_c_ Bars
  C_full parse_strip_c_ C_raw
  // text.parse extracts R.0.tail. Mirror that for C side.
  S_str dump S_full
  C_tail if C_full.end then [[]] else C_full.0.tail
  C_str dump C_tail
  if S_str >< C_str:
    say "  OK   [Label]"
  else
    say "  FAIL [Label]"
    say "    in:  [Src]"
    say "    sym: [S_str]"
    say "    c:   [C_str]"

probe "atom-int" "1"
probe "atom-sym" "X"
probe "binary-plus" "X+Y"
probe "binary-mul-div" "X*Y/Z"
probe "binary-precedence" "1+2*3"
probe "method-dot" "X.f"
probe "method-caret" "X^f"
probe "parens" "(1+2)"
probe "brackets" "\[1 2 3]"
probe "inline-if" "if X: A"
probe "inline-if-else" "if X: A else B"
probe "if-then" "if X then A else B"
probe "say-call" "say X"
probe "splice-string" 'say "hello"'
probe "func-def" "f X = X + 1"
probe "func-call" "f(X Y)"
probe "list-literal" "Xs: 1 2 3"
