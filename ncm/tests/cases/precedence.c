// Operator precedence sanity, including the just-fixed << >> and
// && || ranks.  Higher precedence binds tighter.
//
//   parens > unary > * / % > + - > << >> > < > <= >=
//   > == != > & | ^ > && ||
mul_first  = #[1 + 2 * 3]
shift_lt_add = #[1 + 2 << 3]
shift_lt_cmp = #[1 << 2 < 5]
and_lt_or  = #[1 && 0 || 1]
hex_arith  = #[0x10 + 0x20]
log2_call  = #[log2(1024)]
unary      = #[-5 + 10]
complex    = #[(1 + 2) * 3 << 1]
