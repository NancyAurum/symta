// 05-lambda.s -- lambda arrows and function-definition shape.

show Src =
  R Src.parse('t')
  say "  [R]"

say "single-arg arrow:"
show "X => X + 1"

say "multi-arg arrow:"
show "(X Y) => X + Y"
show "X Y => X + Y"

say "anonymous in call:"
show "Xs.map(X => X*X)"

say "explicit function definition (`Name Args = Body`):"
show "f X = X + 1"
show "g X Y = X * Y"

say "function with multi-line body via `|`:"
show "h X =
  | Y X * 2
  | Y + 1"

say "method definition on a type via `T.`name:"
show "list.double = Me.map(X => X*2)"
