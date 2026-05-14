// 13-core4-else-at-body.s -- CORE-4 regression.
//
// In an `if X:` body, an `else` (or `elif`) at the body's
// indentation column must escape the body so parse_if can
// attach it as the if's trailing clause.  Previously the else
// got absorbed into the body's last statement and parse_tokens
// died with "unexpected else" at the else's position.
//
// 06-offside already covers the case where `else` sits at the
// *if's* column (one less indent than the body); this file
// pins the trickier case where `else` aligns with the body
// itself, and verifies that an inner one-line `if X: Y` `else Z`
// at body indent still binds to the inner if (via the
// "current segment starts with `if`" suppression in
// parse_offside).

show Src =
  R Src.parse('t')
  say "  [R]"

say "else at body indent (CORE-4 reproducer):"
show "f X =
  if X > 0:
    A
    B
    else C"

say "elif at body indent:"
show "f X =
  if X > 10:
    A
    elif X > 0:
      B
    else
      C"

say "nested one-line if/else inside body keeps inner binding:"
show "f =
  if A:
    if X: 1
    else 2
  else 3"
