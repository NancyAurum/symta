// 10-positions.s -- source-position preservation.
// Reader output should retain row/col on tokens for stack traces.

show Src =
  Tk Src.tokenize('mysrc')
  Bars add_bars_c_ Tk
  // Don't strip -- inspect token positions directly.
  for Q Bars:
    when Q.is_token: say "  row=[Q.row] col=[Q.col] type=[Q.type] val=[Q.value]"

say "first line tokens:"
show "X 1"

say "second-line position:"
show "X 1
Y 2"

say "col tracks within line:"
show "abcd efgh"
