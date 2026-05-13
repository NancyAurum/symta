// 09-tilde-question.s -- ~ and ? tokens.
//
// Symta uses `~` and `?` for special functionality:
//   * `?` -- the auto-arg in quick lambdas (Xs{?*?}).
//   * `??` -- the second auto-arg.
//   * `~Var` -- the auto-closure variable.
//   * `~?` -- the auto-closure of `?`.
//   * `~~` -- the iteration counter inside a quick lambda.
//   * `~` alone -- a special token used in various contexts.
//
// All of these have to round-trip through the tokenizer as their
// own distinct tokens so the parser can distinguish them.

dump_tokens Src =
  Tk Src.tokenize('t')
  for Q Tk: say "  [Q.type] [Q.value]"

say "single ?:"
dump_tokens "?"

say "double ??:"
dump_tokens "??"

say "tilde alone:"
dump_tokens "~"

say "tilde-prefixed identifier:"
dump_tokens "~D"
dump_tokens "~Var"

say "name-prefixed by tilde, suffixed by something:"
dump_tokens "n~"
dump_tokens "i~"

say "double tilde (iteration counter):"
dump_tokens "~~"

say "tilde plus dot plus identifier:"
dump_tokens "~D.X"

say "tilde plus question:"
dump_tokens "~?"

say "tilde plus ?+ (auto-closure increment):"
dump_tokens "~i+"

say "tilde collect form:"
dump_tokens "~x.Q"
dump_tokens '~x[Q]'

say "tilde with default:"
dump_tokens "~var^Default"

say "single ? alone vs with operator:"
dump_tokens "?+"
dump_tokens "?*?"
