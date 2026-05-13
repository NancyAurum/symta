// 01-atoms.s -- single-atom parse results.
// text.parse returns a list of forms; each call should give a
// one-element list wrapping the atom (or the atom-as-list for
// post-strip token types).

show Src =
  R Src.parse('t')
  say "  [R]"

say "integer:"
show "0"
show "42"

say "hex:"
show "0xFF"

// Symta uses `Nx...` radix prefixes (2x for binary, 0x for hex);
// `0b...` is NOT a binary literal -- the `0` would be the int and
// the `b` would start an identifier. Test with the canonical form.
say "binary:"
show "2x1010"

say "float (synthesised from int . int):"
show "3.14"
show "0.5"

say "symbol (lowercase):"
show "foo"

say "symbol (uppercase first):"
show "Foo"

say "quoted single word:"
show "\\Foo"

say "single-quoted text:"
show "'hello'"

say "double-quoted string (splice, no incut):"
show "\"hello\""
