// 02-binary.s -- binary-operator precedence + associativity.

show Src =
  R Src.parse('t')
  say "  [R]"

say "single op:"
show "1+2"
show "X*Y"
show "X<Y"

say "left-to-right within same precedence:"
show "1+2+3"
show "X*Y*Z"

say "precedence:  +/- < */%:"
show "1+2*3"
show "1*2+3"

say "comparison:"
show "X >< Y"
show "X << Y"
show "X >> Y"

say "boolean (and/or are infix):"
show "X and Y"
show "X or Y"
show "X and Y or Z"

say "bitwise (-+-, -*-, ...):"
show "X -+- Y"
show "X -*- Y"
