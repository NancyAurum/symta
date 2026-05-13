// 12-curly.s -- the `{}` map / pattern macro in its many forms.
//
// `Xs{...}` is the workhorse: map, filter, reduce, group, accumulate.
// The body's shape decides what happens. See sbe.txt's "Looping" and
// "Advanced List-processing constructs" chapters. The READER's job
// is just to bundle the `{}` body into a `{` token whose payload is
// the inner forms; the macro layer interprets it.

show Src =
  R Src.parse('t')
  say "  [R]"

say "plain map (lambda):"
show 'Xs{X => X*X}'

say "quick-lambda with `?`:"
show 'Xs{?*?}'
show 'Xs{? > 5}'

say "method call on each element:"
show 'Xs{method_name}'

say "keep/skip (case form):"
show 'Xs{?<5=}'
show 'Xs{:?<5}'

say "literal-replace cases:"
show 'Xs{1=0; 0=1}'
show 'Xs{1=0; 0=1; _=No}'

say "auto-closure accumulator:"
show 'Xs{&~s+?}'
show 'Xs{~D.?+}'

say "auto-closure counter and named:"
show 'Xs{~i+}'
show 'Xs{~j.Q : ?%2 = ~i.Q}'

say "group N:"
show 'Xs{~/3}'
show 'Xs{N/L = L}'

say "destructure within map:"
show 'Xs{A B C}'
show 'Xs{[A B] = [B A]}'
show 'Xs{R G B = R G B}'

say "splat replacement (`@`):"
show 'Xs{@[1 2 3] = @[4 5 6]}'

say "select index inside map:"
show 'Xs{?,5+}'
