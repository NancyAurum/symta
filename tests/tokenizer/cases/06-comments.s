// 06-comments.s -- comments are stripped entirely by the tokenizer.

count Src =
  Tk Src.tokenize('t')
  say "  [Tk.n] tokens"
  for Q Tk: say "    [Q.type] [Q.value]"

say "line comment only -- yields zero tokens:"
count "// just a comment"

say "line comment after code -- code retained, comment dropped:"
count "x // trailing"

say "block comment inline:"
count "a /* b */ c"

say "block comment spanning lines:"
count "x
/* multi
   line */
y"

say "comment-like inside string -- preserved as text:"
count "'// not a comment'"
