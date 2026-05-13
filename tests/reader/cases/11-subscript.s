// 11-subscript.s -- advanced Xs[...] subscript / slicing forms.
//
// The `[...]` postfix is heavily overloaded -- single index, slice,
// multi-index, replacement (`N!V`), insertion (`@N!V`), negative
// index (`-I`), `~` for last, range with step. See sbe.txt's
// "Advanced List Operations" chapter. The reader has to parse all
// of these as suffix-call forms with the inner tokens grouped under
// the `[]` token type.
//
// Use single-quoted strings throughout so the test inputs don't get
// eaten by double-quote splice interpolation.

show Src =
  R Src.parse('t')
  say "  [R]"

say "single index:"
show 'Xs[0]'
show 'Xs[A+B]'

say "slice:"
show 'Xs[N:M]'
show 'Xs[:M]'
show 'Xs[N:]'

say "slice with step:"
show 'Xs[0::2]'
show 'Xs[20:-10:2]'

say "last-element shortcut:"
show 'Xs[~]'
show 'Xs[~:0]'

say "negative index:"
show 'Xs[-I]'

say "multi-index:"
show 'Xs[A B C]'
show 'Xs[A B C D N]'

say "replacement at index (N!V):"
show 'Xs[N!V]'
show 'Xs[N!V M!U]'

say "splice replacement (N!@Ys):"
show 'Xs[N!@Ys]'

say "additive insert (@N!V):"
show 'Xs[@N!V]'

say "locate (3!):"
show 'Xs[3!]'

say "error-suppressing form (!...):"
show 'Xs[!N:M]'
show 'Xs[!3]'
